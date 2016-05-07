/**
 * Created by fripside on 5/6/16.
 */
#include <cstdio>
#include "string"
#include "../Socket.h"

using namespace std;
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

void testSocket() {
    puts("startSocket");
    TCPSocket tcpSocket;
    tcpSocket.Create(AF_INET);
//    SockAddr addr = SockAddr("192.168.57.1", 9091);
//    tcpSocket.Bind(addr);
    int port = tcpSocket.TryBindHost(9001, 9099);
    printf("start listen: %d\n", port);
//    tcpSocket.SetNonBlock(true);
    tcpSocket.Listen();
    puts("start accept...");
//    string host = SockAddr::GetHostName();
//    printf("start sever: %s", host.c_str());
    TCPSocket client = tcpSocket.Accept();
    int n = 5;
    char sendBuf[100], recvBuf[100];
    while (n--) {
        recvStr(client, recvBuf);
        printf("RECV: %s\n", recvBuf);
        sendStr(client, recvBuf);
        printf("SEND: %s\n", recvBuf);
    }
}

int main() {
    testSocket();
    return 0;
}

/**测试数据：

**/