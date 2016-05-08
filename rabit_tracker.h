//
// Created by fripSide on 4/12/16.
//

#ifndef RABIT_RABIT_TRACKER_H
#define RABIT_RABIT_TRACKER_H

#include <assert.h>
#include <set>
#include <algorithm>
#include "Socket.h"
#include "cstdio"
#include "string"
#include "pthread.h"
#include "map"

using namespace rabit::utils;
using namespace std;

const bool LOG_TO_FILE = true;
const bool LOG_TO_CONSOLE = true;

typedef void* (*submitFunc)(void *);
struct submitArgs {
    int nSlave;
    string cmd;
    bool verbose;
    vector<string> workerArgs;
    map<string, string> workerEnv;
    submitArgs(int nworks) {
        nSlave = nworks;
    }
    submitArgs(int nworks, vector<string> args, map<string, string> envs) {
        nSlave = nworks;
        workerArgs = args;
        workerEnv = envs;
    }
    string getArgs() {
        string str = "";
        for (int i = 0; i < workerArgs.size(); ++i) {
            str += " " + workerArgs[i];
        }
        map<string, string>::iterator iter = workerEnv.begin();
        while (iter != workerEnv.end()) {
            str += " " + iter->first + "=" + iter->second;
            ++iter;
        }
        return str;
    }
};


const bool DEBUG = true;

void debug_print(const char *fmt,...) {
    if (!DEBUG)
        return;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt,ap);
    fflush(stdout);
    va_end(ap);
}

void log_print(const char *fmt,...) {
    va_list ap;
    va_start(ap, fmt);
    if (LOG_TO_FILE) {
        FILE* fp = fopen("tracker_log.txt","a");
        vfprintf(fp, fmt, ap);
    }

    if (LOG_TO_CONSOLE){
        vprintf(fmt,ap);
        debug_print(fmt, ap);
        fflush(stdout);
    }
    va_end(ap);
}


const int KMAGIC = 0xff99;

class ExSocket {
public:
    ExSocket(TCPSocket *s): sock(*s) {
    }

    size_t sendInt(int t) {
        char str[8];
        sprintf(str, "%d", t);
        size_t s = sock.SendAll(str, 4);
        return s;
    }

    void sendStr(string str) {
        sock.SendAll(str.data(), str.length());
    }

    int recvInt() {
        char str[58];
        sock.RecvAll(str, 4);
        int t = atoi(str);
        return t;
    }


    string recvStr() {
        int len = recvInt();
        string str;
        size_t t = sock.RecvAll(&(str[0]), len);
        return str;
    }

private:
    TCPSocket &sock;

};

class SlaveEntry {
public:
    SlaveEntry(TCPSocket *tcpSock, const char *sAddr): sock(tcpSock) {
//        hostIp = SockAddr:
        int magic = sock.recvInt();
        char err[50];
        host = gethostbyname(sAddr);
        sprintf(err, "invalid magic number=%d from %s", magic, host->h_name);
        Assert(magic == KMAGIC, err);
        sock.sendInt(KMAGIC);
        rank = sock.recvInt();
        worldSize = sock.recvInt();
        jobId = sock.recvStr();
        cmd = sock.recvStr();
    }

    int decide_rank(map<string, int> & jobMap) {
        if (rank >= 0) {
            return rank;
        }
        if (jobId.compare("NULL") != 0 && jobMap.find(jobId) != jobMap.end()) {
            return jobMap[jobId];
        }
        return -1;
    }

    vector<int> assign_rank(int rank, map<int, SlaveEntry*> &waitConn, map<int, vector<int> > treeMap, map<int, int> & parentMap, map<int, pair<int, int> > &ringMap) {
        this->rank = rank;
        set<int> nnSet = set<int>(treeMap[rank].begin(), treeMap[rank].end());
        int rprev = ringMap[rank].first, rnext = ringMap[rank].second;

        sock.sendInt(rank);
        sock.sendInt(parentMap[rank]);
        sock.sendInt((int) treeMap.size());
        sock.sendInt((int) nnSet.size());

        for (set<int>::iterator iter = nnSet.begin(); iter != nnSet.end(); ++iter) {
            sock.sendInt(*iter);
        }

        if (rprev != -1 && rprev != rank) {
            nnSet.insert(rprev);
            sock.sendInt(rprev);
        } else {
            sock.sendInt(-1);
        }

        if (rnext != -1 && rnext != rank) {
            nnSet.insert(rnext);
            sock.sendInt(rnext);
        } else {
            sock.sendInt(-1);
        }

        while (true) {
            int nGood = sock.recvInt();
            set<int> goodSet = set<int>();
            for (int i = 0; i < nGood; ++i) {
                goodSet.insert(sock.recvInt());
            }
            set<int> badSet;
            set_intersection(nnSet.begin(), nnSet.end(), goodSet.begin(), goodSet.end(), inserter(badSet, badSet.begin()));

            vector<int> conSet;
            for (set<int>::iterator iter = badSet.begin(); iter != badSet.end(); ++iter) {
                if (waitConn[*iter] != NULL) {
                    conSet.push_back(*iter);
                }
            }
            sock.sendInt((int) conSet.size());
            int left = ((int) badSet.size()) - (int) conSet.size();
            sock.sendInt(left);

            for (int i = 0; i < conSet.size(); ++i) {
                sock.sendStr(waitConn[conSet[i]]->hostIp);
                sock.sendInt(waitConn[conSet[i]]->port);
                sock.sendInt(conSet[i]);
            }
            int nErr = sock.recvInt();
            if (nErr != 0) continue;
            port = sock.recvInt();
            vector<int> rmSet;
            for (int i = 0; i < conSet.size(); ++i) {
                int r = conSet[i];
                waitConn[r]->waitAccept -= 1;
                if (waitConn[r]->waitAccept == 0) rmSet.push_back(r);
            }
            for (int i = 0; i < rmSet.size(); ++i) {
                map<int, SlaveEntry*>::iterator iter = waitConn.find(rmSet[i]);
                waitConn.erase(iter);
            }
            waitAccept = ((int) badSet.size()) - (int) conSet.size();
            return rmSet;
        }
    }

    ExSocket sock;

private:
    int port;
    string hostIp;
    int rank;
    int worldSize;
    hostent * host;
    string jobId;
    string cmd;
    int waitAccept;
};

struct LinkMap {
    map<int, vector<int> > treeMap;
    map<int, pair<int, int> > ringMap;

};

class Tracker {
public:
    Tracker(string url, int start_port = 9091, int end_port = 9999, bool verbose = true, const string &hostIp = "auto"): port(-1) {
//        sock = TCPSocket();
//        TCPSocket tcpSocket;
//        tcpSocket.Create();
//        printf("CREATE %d\n", tcpSocket.sockfd);
//        int port = -1;
////        string url("192.168.57.1");
////    SockAddr addr(url.data(), 9099);
//        hostent *hp = gethostbyname(url.data());
//        printf("%s %s\n", hp->h_addr_list[0], hp->h_name);
////    printf("%s %d\n", addr.AddrStr().data(), addr.port());
////    tcpSocket.Bind(addr);
//        for (int p = 9090; p < 9999; ++p) {
//            SockAddr addr = SockAddr(url.data(), p);
//            if (bind(tcpSocket.sockfd, reinterpret_cast<const sockaddr*>(&addr.addr),
//                     sizeof(addr.addr)) == 0) {
//                port = p;
//                break;
//            }
//            printf("TryBind %s:%d Failed\n", addr.AddrStr().data(), addr.port());
//        }
//        if (port == -1) {
//            printf("Bind Port failed %s:%d", url.data(), port);
//            exit(-1);
//        }

//        printf("start listen: %d\n", port);
        sock.Create();
        port = sock.TryBindHost(start_port, end_port);
        printf("Bind Port %d\n", port);
//        printf("CREATE %d\n", sock.sockfd);
//        for (int p = start_port; p < end_port; ++p) {
//            SockAddr addr(url.data(), p);
//            if (bind(sock.sockfd, reinterpret_cast<sockaddr*>(&addr.addr),
//                     sizeof(addr.addr)) == 0) {
//                printf("Bind Port Success %s:%d\n", url.data(), p);
//                port = p;
//                break;
//            } else {
//                printf("Bind Port Failed %s:%d\n", addr.AddrStr().data(), addr.port());
//            }
//        }
//        if (port == -1) {
//            printf("Bind Port failed %s:%d\n", url.data(), port);
//            exit(-1);
//        }
        sock.Listen();
        this->hostIp = url;
        if (hostIp.compare("auto") == 0) {
            this->hostIp = url;
            printf("start listen on: %d\n", port);
        }
        printf("start listen on %s:%d", url.data(), port);
    }

    ~Tracker() {
        sock.Close();
    }

    map<string, string> slaveEnvs() {
        map<string, string> t;
        if (hostIp.compare("dns") == 0) {
            host = SockAddr::GetHostName().c_str();
        } else if (hostIp.compare("ip") == 0) {
            host = SockAddr::GetHostName().c_str();
        } else {
            host = hostIp;
        }
        t["rabit_tracker_uri"] = host;
        char p[50];
        sprintf(p, "%d", port);
        t["rabit_tracker_port"] = p;
        return t;
    }

    vector<int> getNeighbor(int rank, int nSlave) {
        vector<int> nes;
        rank = rank + 1;
        if (rank > 1) {
            nes.push_back(rank / 2 - 1);
        }
        if (rank *2 - 1 < nSlave) {
            nes.push_back(rank * 2 - 1);
        }
        if (rank * 2 < nSlave) {
            nes.push_back(rank * 2);
        }
        return nes;
    }

    map<int, pair<int, int> > getRing(LinkMap &linkMap) {
        map<int, pair<int, int> > ringMap;
        int nSlave = (int) linkMap.treeMap.size();
        for (int i = 0; i < nSlave; ++i) {
            int rprev = (i + nSlave - 1) % nSlave;
            int rnext = (i + 1) % nSlave;
            ringMap[i] = pair<int, int>(rprev, rnext);
        }
        return ringMap;
    }

    LinkMap getLinkMap(int nSlave) {
        LinkMap linkMap;
        linkMap.ringMap = getRing(linkMap);

        return linkMap;
    }

    void acceptSlaves(int nSlave) {
        map<int, int> shutDown;
        string adr("");
        map<string, int> jobMap;
        while (shutDown.size() != nSlave) {
            TCPSocket sc = sock.Accept();
            SlaveEntry se(&sc, adr.data());
            int rank = se.decide_rank(jobMap);
            log_print("@tracker All of %d nodes getting started", nSlave, 2);
        }
    }

private:
    TCPSocket sock;
    string hostIp;
    int port;
    string host;
};

void submit(submitArgs *args, string url, submitFunc sub, bool verbose, string hostIp = "auto") {
    Tracker master(url, 9091, 9999, verbose, hostIp);
    printf("Submit...");
    pthread_t td;
    args->workerEnv =  master.slaveEnvs();
    args->workerEnv["env-key"] = "test-key";
    pthread_create(&td, NULL, sub, args);
    pthread_join(td, NULL);
}

#endif //RABIT_RABIT_TRACKER_H
