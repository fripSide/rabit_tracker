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
    printf("treeMap %u\n", 5);
    LinkMap linkMap = master.getLinkMap(5);
    printf("treeMap %lu\n", linkMap.treeMap.size());
    map<int, vector<int> > &treeMap = linkMap.treeMap;
    for (int i = 0; i < treeMap.size(); ++i) {
        printf("size: %lu\n", treeMap[i].size());
        for (int j = 0; j < treeMap[i].size(); ++j) {
            printf("%d ", treeMap[i][j]);
        }
        printf("\n");
    }
//    master.slaveEnvs();
//    master.acceptSlaves(5);
    return 0;
}

/**测试数据：

**/