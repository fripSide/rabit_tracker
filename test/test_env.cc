/**
 * Created by fripside on 5/4/16.
 */
# include <cstdio>
#include <stdlib.h>


const char *KEY = "env-key";
extern char** environ;

int main(int argc, char * argv[]) {
    printf("ARGV: %s \n", argv[0]);
    char * str = getenv(KEY);
//    puts(environ[0]);
    printf("ENV: %s \n", str);
    long long sum = 0;
    for (int i = 0; i < 99999; ++i) {
        for (int j = 0; j < 999; ++j) {
            sum += i * (long long) j ;
        }
    }
    printf("finished %s sum=%lld\n", argv[0], sum);
    return 0;
}

/**测试数据：

**/