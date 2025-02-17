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

    int user_data = 42;
    putLRUCache(cache, "user:123", &user_data, 5);  // 5초 후 만료

    sleep(3);
    int *data = (int*)getLRUCache(cache, "user:123");
    if (data) {
        printf("Data: %d\n", *data);  // 출력: Data: 42
    } else {
        printf("Data expired or not found.\n");
    }

    // 5초 후 데이터 접근 (만료됨)
    sleep(5);
    data = (int*)getLRUCache(cache, "user:123");
    if (data) {
        printf("Data: %d\n", *data);
    } else {
        printf("Data expired or not found.\n");  // 출력: Data expired or not found.
    }
    /*
    printf("benchmark started \n");
    // 성능 테스트
    benchmark_test(cache);

    printf("benchmark finished\n");
    */
    destroy_LRUCache(cache);

    printf("destroy_LRUCache finished\n");

    return 0;
}