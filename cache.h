#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>

struct LRUNode;

struct LRUNode {
    char *key;
    void *value;
    struct LRUNode *prev;
    struct LRUNode *next;
    time_t expire;
};

struct LRUCache {
    struct HashTable *table;
    struct LRUNode *head;    
    struct LRUNode *tail;    
    size_t max_items;
    size_t current_items;
    pthread_mutex_t lock;  // 동시성 제어
    pthread_t cleaner_thread;  
    int is_running;            
};

struct LRUCache* new_LRUCache_with_max_items(size_t max_items);
void destroy_LRUCache(struct LRUCache* cache);
void putLRUCache(struct LRUCache* cache, const char *key, void *value, int ttl);
void *getLRUCache(struct LRUCache* cache, const char *key);

#endif //CACHE_H
