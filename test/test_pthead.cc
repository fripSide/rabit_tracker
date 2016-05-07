/**
 * Created by fripside on 5/5/16.
 */
# include <cstdio>
#include "pthread.h"

const int MAX_N = 100;

void* run(void * args) {
    long long sum = 0;
    for (int i = 0; i < 99999; ++i) {
        for (int j = 0; j < 999; ++j) {
            sum += i * (long long) j ;
        }
    }
    printf("finished %s sum=%lld\n", args, sum);
    return NULL;
}

int main() {
    if (freopen("/Users/fripside/ClionProjects/cc150/input", "r", stdin) == NULL) {
        fprintf(stderr, "error redirecting stdout\n");
    }
    pthread_t tid1;
    char args[10] = "1232sd";
    pthread_create(&tid1, NULL, run, args);
    pthread_join(tid1, NULL);
    printf("aaaa %s", "end thread");
    return 0;
}

/**测试数据：

**/