#include "hash.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROBE_DEPTH 15
#define LOAD_FACTOR 0.7
#define INITIAL_CAPACITY 16

struct CacheEntry {
    char *key;
    void *value;
    size_t probe_distance;  // Robin Hood hashing용 프로브 거리
    time_t expire;          // TTL management
};

struct HashTable {
    struct CacheEntry *entries;
    size_t size;
    size_t capacity;
    pthread_rwlock_t rwlock;  // Read-Write Lock
};

unsigned int hash(const char *key) {
    unsigned int hashval = 5381;
    int c;
    while ((c = *key++)) {
        hashval = ((hashval << 5) + hashval) + c;
    }
    return hashval;
}

// Robin Hood 해싱 삽입 로직
size_t robin_hood_probe(struct HashTable *table, const char *key, size_t *probe_count) {
    size_t index = hash(key) % table->capacity;
    *probe_count = 0;

    while (*probe_count < MAX_PROBE_DEPTH) {
        if (table->entries[index].key == NULL ||
            strcmp(table->entries[index].key, key) == 0) {
            return index;
        }

        // 현재 엔트리의 프로브 거리가 새로운 엔트리보다 작으면 교체
        if (table->entries[index].probe_distance < *probe_count) {
            return index;  // 교체 대상 인덱스
        }

        index = (index + 1) % table->capacity;
        (*probe_count)++;
    }
    return -1;  // probe failed
}

// Read-Write Lock 기반 해시 테이블 조회
void *hash_table_get(struct HashTable *table, const char *key) {
    pthread_rwlock_rdlock(&table->rwlock);

    size_t probe_count = 0;
    size_t index = robin_hood_probe(table, key, &probe_count);

    void *result = NULL;
    if (index != (size_t)-1 && table->entries[index].key != NULL) {
        result = table->entries[index].value;
    }

    pthread_rwlock_unlock(&table->rwlock);
    return result;
}

// 동적 리사이징 함수
void hash_table_resize(struct HashTable *table) {
    size_t new_cap = table->capacity * 2;
    struct CacheEntry *new_entries = calloc(new_cap, sizeof(struct CacheEntry));

    for (size_t i = 0; i < table->capacity; i++) {
        if (table->entries[i].key) {
            size_t probe_count = 0;
            size_t new_idx = robin_hood_probe(table, table->entries[i].key, &probe_count);

            if (new_idx != (size_t)-1) {
                new_entries[new_idx] = table->entries[i];
                new_entries[new_idx].probe_distance = probe_count;
            }
        }
    }

    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_cap;
}

// RCU(Read-Copy-Update) 패턴 적용 삽입
void hash_table_put_rcu(struct HashTable *table, const char *key, void *value, int ttl) {
    pthread_rwlock_wrlock(&table->rwlock);

    if ((float)table->size / table->capacity > LOAD_FACTOR) {
        hash_table_resize(table);
    }

    size_t probe_count = 0;
    size_t index = robin_hood_probe(table, key, &probe_count);

    if (index != (size_t)-1) {
        if (table->entries[index].key) {
            // 기존 항목 업데이트
            free(table->entries[index].key);
        } else {
            table->size++;
        }

        table->entries[index].key = strdup(key);
        table->entries[index].value = value;
        table->entries[index].probe_distance = probe_count;
        table->entries[index].expire = time(NULL) + ttl;
    }

    pthread_rwlock_unlock(&table->rwlock);
}

struct HashTable* new_hash_table_with_default() {
    struct HashTable* table = calloc(1, sizeof(struct HashTable));

    table->entries = calloc(INITIAL_CAPACITY, sizeof(struct CacheEntry));
    table->capacity = INITIAL_CAPACITY;
    table->size = 0;

    pthread_rwlock_init(&table->rwlock, NULL);

    return table;
}

void destroy_hash_table(struct HashTable *table) {
  pthread_rwlock_destroy(&table->rwlock);
  free(table->entries);
  free(table);
}
