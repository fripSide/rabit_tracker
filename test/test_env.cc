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
    return 0;
}

/**测试数据：

**/