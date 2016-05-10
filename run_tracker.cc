/**
 * Created by fripside on 5/9/16.
 */
#include <cstdio>
#include "rabit_tracker.h"
#include "string"

using namespace std;

int main(int argv, char * args[]) {
//    if (argv < 2) {
//        printf("Usage: run ip\n");
//        exit(0);
//    }
    std::string url = "10.11.53.64";
    Tracker master(url, 9091, 9999, true);
    LinkMap linkMap = master.getLinkMap(5);
//    master.slaveEnvs();
//    master.acceptSlaves(5);
    return 0;
}

/**测试数据：

**/