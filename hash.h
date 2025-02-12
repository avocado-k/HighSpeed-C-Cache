#ifndef HASH_H
#define HASH_H

struct HashTable;

void hash_table_remove(struct HashTable *table, const char *key);
void hash_table_put_rcu(struct HashTable *table, const char *key, void *value, int ttl);
void *hash_table_get(struct HashTable *table, const char *key);
struct HashTable* new_hash_table_with_default();
void destroy_hash_table(struct HashTable *table);

#endif //HASH_H
