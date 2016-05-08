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
    tcpSocket.Create();
    printf("CREATE %d\n", tcpSocket.sockfd);
    int port = -1;
    string url("192.168.57.1");
//    SockAddr addr(url.data(), 9099);
    hostent *hp = gethostbyname(url.data());
    printf("%s %s\n", hp->h_addr_list[0], hp->h_name);
//    printf("%s %d\n", addr.AddrStr().data(), addr.port());
//    tcpSocket.Bind(addr);
    SockAddr addr;
    for (int p = 9090; p < 9999; ++p) {
        addr = SockAddr(url.data(), p);
        if (bind(tcpSocket.sockfd, reinterpret_cast<const sockaddr*>(&addr.addr),
                 sizeof(addr.addr)) == 0) {
            port = p;
            break;
        }
        printf("TryBind %s:%d Failed\n", addr.AddrStr().data(), addr.port());
    }
    if (port == -1) {
        printf("Bind Port failed %s:%d", url.data(), port);
        exit(-1);
    }

//    int port = tcpSocket.TryBindHost(9001, 9099);
    printf("start listen: %s:%d\n", addr.AddrStr().data(), port);
//    tcpSocket.SetNonBlock(true);
    tcpSocket.Listen();
    puts("start accept...");
//    string host = SockAddr::GetHostName();
//    printf("start sever: %s", host.c_str());
    TCPSocket client = tcpSocket.Accept();
    printf("RECV CLIENT: %s:%d\n", client.sAddr.AddrStr().data(), client.sAddr.port());
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