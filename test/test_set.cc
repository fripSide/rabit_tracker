/**
 * Created by fripside on 5/8/16.
 */
# include <cstdio>
#include "set"
#include "vector"

using namespace std;

const int MAX_N = 100;

// g++ -std=c++11 -stdlib=libc++ test_set.cc -o tset

void solve() {
    puts("solve");
    set<int> s1;
    s1.insert(2), s1.insert(3), s1.insert(5);
    set<int> s2;
    s2.insert(5), s2.insert(6), s2.insert(7);
    set<int> s3;
    set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s3, s3.begin()));

    printf("set_intersection\n"); //交集
    set<int>::iterator iter = s3.begin();
    while (iter != s3.end()) {
        printf("%d\n", *iter);
        ++iter;
    }

    s3.clear();
    set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s3, s3.begin()));

    printf("set_difference\n"); //交集
    iter = s3.begin();
    while (iter != s3.end()) {
        printf("%d\n", *iter);
        ++iter;
    }

    printf("\n");

    vector<int> v1 = {2, 3, 4, 5};
    vector<int> v2 = {8, 9, 10};
    v1.insert(v1.end(), v2.begin(), v2.end());
    printf("vector insert:\n");
    for (int i = 0; i < v1.size(); ++i) {
        printf("%d ", v1[i]);
    }
    int v = v1.back();
    printf("\n %d\n", v);
}


int main() {
    if (freopen("/Users/fripside/ClionProjects/cc150/input", "r", stdin) == NULL) {
        fprintf(stderr, "error redirecting stdout\n");
    }
    solve();
    return 0;
}

/**测试数据：

**/