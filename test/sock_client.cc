/**
 * Created by fripside on 5/6/16.
 */
# include <cstdio>
#include "cstring"
#include "../Socket.h"

using namespace rabit::utils;

const int MAX_N = 100;

void sendInt(TCPSocket &sock, int t) {
    sock.SendAll(&t, sizeof t);
}

int recvInt(TCPSocket &sock) {
    int t;
    sock.RecvAll(&t, sizeof t);
    return t;
}

int sendStr(TCPSocket &sock, char *s) {
    size_t t = strlen(s);
    sendInt(sock, t);
    sock.SendAll(s, t);
    return (int) t;
}

int recvStr(TCPSocket &sock, char *s) {
    int t = recvInt(sock);
    sock.RecvAll(s, t);
//    printf("RECV: %d %s", t, s);
    s[t] = 0;
    return t;
}

void startSocket() {
    TCPSocket client = TCPSocket();
    client.Create(AF_INET);
    SockAddr addr("0.0.0.0", 9003);
    bool con = client.Connect(addr);
    if (!con) {
        printf("Connect failed!");
    } else {
        printf("Connect suc!");
    }
    char sendBuf[100], recvBuf[100];
    int n = 5;
    while (n--) {
        printf("Input send buf: ");
        scanf("%s", sendBuf);
        sendStr(client, sendBuf);
        printf("SEND: %s\n", sendBuf);
        recvStr(client, recvBuf);
        printf("%s\n", recvBuf);
    }
}


int main() {
    startSocket();
    return 0;
}

/**测试数据：

**/