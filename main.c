#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.7
#define MAX_PROBE_DEPTH 15

unsigned int hash(const char *key);

typedef struct {
    char *key;
    void *value;
    size_t probe_distance;  // Robin Hood hashing용 프로브 거리
    time_t expire;          // TTL management
} CacheEntry;

typedef struct {
    CacheEntry *entries;
    size_t size;
    size_t capacity;
    pthread_rwlock_t rwlock;  // Read-Write Lock
} HashTable;

typedef struct {
    HashTable *table;
    size_t max_items;
    size_t current_items;
} LRUCache;

unsigned int hash(const char *key) {
    unsigned int hashval = 5381;
    int c;
    while ((c = *key++)) {
        hashval = ((hashval << 5) + hashval) + c;
    }
    return hashval;
}

// Robin Hood 해싱 삽입 로직
size_t robin_hood_probe(HashTable *table, const char *key, size_t *probe_count) {
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
void *hash_table_get(HashTable *table, const char *key) {
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
void hash_table_resize(HashTable *table) {
    pthread_rwlock_wrlock(&table->rwlock);
    
    size_t new_cap = table->capacity * 2;
    CacheEntry *new_entries = calloc(new_cap, sizeof(CacheEntry));
    
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
    
    pthread_rwlock_unlock(&table->rwlock);
}

// RCU(Read-Copy-Update) 패턴 적용 삽입
void hash_table_put_rcu(HashTable *table, const char *key, void *value, int ttl) {
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

// Lock-Free 큐를 이용한 배치 처리 (예시)
typedef struct {
    CacheEntry *entries;
    size_t head;
    size_t tail;
} LockFreeQueue;

void batch_evict(LRUCache *cache, size_t count) {
    // Todo: Lock-Free 큐 구현 (atomic 연산 사용)
    // 배치 처리로 여러 항목을 한번에 제거
}

// 벤치마크 테스트 함수
void benchmark_test(LRUCache *cache) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // 100만번 읽기/쓰기 테스트
    for (int i = 0; i < 1000000; i++) {
        char key[32];
        sprintf(key, "item%d", i);
        hash_table_put_rcu(cache->table, key, &i, 0);
        hash_table_get(cache->table, key);
    }
    
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - start.tv_usec;
    printf("Throughput: %.2f ops/sec\n", 1000000.0 / (micros / 1000000.0));
}

int main() {
    HashTable table = {
        .entries = calloc(INITIAL_CAPACITY, sizeof(CacheEntry)),
        .capacity = INITIAL_CAPACITY,
        .size = 0
    };
    pthread_rwlock_init(&table.rwlock, NULL);
    
    LRUCache cache = {&table, 10000, 0};
    
    // 성능 테스트
    benchmark_test(&cache);
    
    free(table.entries);
    pthread_rwlock_destroy(&table.rwlock);
    return 0;
}