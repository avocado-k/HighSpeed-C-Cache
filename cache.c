#include "cache.h"
#include "hash.h"

struct LRUCache {
    struct HashTable *table;
    size_t max_items;
    size_t current_items;
};

struct LRUCache *new_cache(struct HashTable *table, size_t max_items) {
  struct LRUCache *cache = calloc(1, sizeof(struct LRUCache));

  cache->table = table;
  cache->max_items = max_items;
  cache->current_items = 0;

  return cache;
}

struct LRUCache* new_LRUCache_with_max_items(size_t max_items) {
  struct HashTable* table = new_hash_table_with_default();

  struct LRUCache* cache = new_cache(table, max_items);

  return cache;
}

void destroy_LRUCache(struct LRUCache* cache) {
  destroy_hash_table(cache->table);
  free(cache);
}

void putLRUCache(struct LRUCache* cache, const char *key, void *value, int ttl) {
    hash_table_put_rcu(cache->table, key, value, ttl);
}

void *getLRUCache(struct LRUCache* cache, const char *key) {
  return hash_table_get(cache->table, key);
}
