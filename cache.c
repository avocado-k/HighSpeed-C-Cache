#include "cache.h"
#include "hash.h"
#include <pthread.h>  
#include <string.h>   
#include <time.h>     
#include <stdio.h>



void _remove_node(struct LRUCache *cache, struct LRUNode *node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache->head) cache->head = node->next;
    if (node == cache->tail) cache->tail = node->prev;
}

static void* _cleanup_thread(void *arg) {
    struct LRUCache *cache = (struct LRUCache*)arg;
    
    while (cache->is_running) {
        sleep(1);  // 1초마다 검사
        pthread_mutex_lock(&cache->lock);
        
        struct LRUNode *current = cache->head;
        while (current != NULL) {
            struct LRUNode *next = current->next;
            if (time(NULL) > current->expire) {
                hash_table_remove(cache->table, current->key);
                _remove_node(cache, current);  // 연결 리스트에서 노드 제거
                free(current->key);
                free(current);
                cache->current_items--;
            }
            current = next;
        }
        
        pthread_mutex_unlock(&cache->lock);
    }
    return NULL;
}


void _add_to_head(struct LRUCache *cache, struct LRUNode *node) {
    node->prev = NULL;
    node->next = cache->head;
    
    if (cache->head != NULL) {
        cache->head->prev = node;
    }
    cache->head = node;
    
    if (cache->tail == NULL) {
        cache->tail = node;
    }
}

void _move_to_head(struct LRUCache *cache, struct LRUNode *node) {
    if (node == cache->head) return;

    // 현재 위치에서 분리
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache->tail) cache->tail = node->prev;

    // 헤드에 삽입
    node->prev = NULL;
    node->next = cache->head;
    if (cache->head) cache->head->prev = node;
    cache->head = node;
}
void putLRUCache(struct LRUCache* cache, const char *key, void *value, int ttl) {
    pthread_mutex_lock(&cache->lock);

    // 1. 기존 노드 존재 여부 확인
    struct LRUNode *existing = (struct LRUNode*)hash_table_get(cache->table, key);
    if (existing) {
        // 기존 노드 업데이트 및 헤드로 이동
        existing->value = value;
        _move_to_head(cache, existing);
        pthread_mutex_unlock(&cache->lock);
        return;
    }

    // 2. 새 노드 생성
    struct LRUNode *new_node = malloc(sizeof(struct LRUNode));
    new_node->key = strdup(key);
    new_node->value = value;
    new_node->expire = time(NULL) + ttl;
    new_node->prev = NULL;
    new_node->next = NULL;

    // 3. 해시 테이블에 저장
    hash_table_put_rcu(cache->table, key, new_node, ttl);
    _add_to_head(cache, new_node);
    cache->current_items++;

    // 4. LRU Eviction
    if (cache->current_items > cache->max_items) {
        struct LRUNode *old_tail = cache->tail;
        cache->tail = old_tail->prev;
        if (cache->tail) cache->tail->next = NULL;
        
        hash_table_remove(cache->table, old_tail->key);
        free(old_tail->key);
        free(old_tail);
        cache->current_items--;
    }

    pthread_mutex_unlock(&cache->lock);
}

struct LRUCache *new_cache(struct HashTable *table, size_t max_items) {
  struct LRUCache *cache = calloc(1, sizeof(struct LRUCache));

  cache->table = table;
  cache->max_items = max_items;
  cache->current_items = 0;

  return cache;
}


void *getLRUCache(struct LRUCache* cache, const char *key) {
    pthread_mutex_lock(&cache->lock);
    struct LRUNode *node = (struct LRUNode*)hash_table_get(cache->table, key);
    
    if (node) {
        _move_to_head(cache, node);  // 접근한 노드를 헤드로 이동
        pthread_mutex_unlock(&cache->lock);
        return node->value;
    }
    
    pthread_mutex_unlock(&cache->lock);
    return NULL;
}

struct LRUCache* new_LRUCache_with_max_items(size_t max_items) {
    struct LRUCache* cache = calloc(1, sizeof(struct LRUCache));
    cache->table = new_hash_table_with_default();
    cache->head = NULL;
    cache->tail = NULL;
    cache->max_items = max_items;
    cache->current_items = 0;
    pthread_mutex_init(&cache->lock, NULL);
    cache->is_running = 1;
    pthread_create(&cache->cleaner_thread, NULL, _cleanup_thread, cache);

    return cache;
}

#define SAFE_FREE(a) do { if (a) { free(a); (a) = NULL; } } while (0)
void destroy_LRUCache(struct LRUCache* cache) {
    cache->is_running = 0;
    pthread_join(cache->cleaner_thread, NULL);  // 스레드 종료 대기
    pthread_mutex_lock(&cache->lock);
    
    struct LRUNode *current = cache->head;
    while (current != NULL) {
        struct LRUNode *next = current->next; // 다음 노드 미리 저장
        if(next != NULL){
          SAFE_FREE(current->key);
          SAFE_FREE(current);
        }
        current = next;
    }
    destroy_hash_table(cache->table);
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    SAFE_FREE(cache);
}