#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>

struct LRUCache;

struct LRUCache* new_LRUCache_with_max_items(size_t max_items);
void destroy_LRUCache(struct LRUCache* cache);
void putLRUCache(struct LRUCache* cache, const char *key, void *value, int ttl);
void *getLRUCache(struct LRUCache* cache, const char *key);

#endif //CACHE_H
