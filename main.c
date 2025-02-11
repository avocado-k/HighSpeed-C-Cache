#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cache.h"

// 벤치마크 테스트 함수
void benchmark_test(struct LRUCache *cache) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 100만번 읽기/쓰기 테스트
    for (int i = 0; i < 1000000; i++) {
        char key[32];
        sprintf(key, "item%d", i);
        putLRUCache(cache, key, &i, 0);
        getLRUCache(cache, key);
    }

    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - start.tv_usec;
    printf("Throughput: %.2f ops/sec\n", 1000000.0 / (micros / 1000000.0));
}

int main() {
    struct LRUCache* cache = new_LRUCache_with_max_items(10000);

    // 성능 테스트
    benchmark_test(cache);

    destroy_LRUCache(cache);

    return 0;
}