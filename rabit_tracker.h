//
// Created by fripSide on 4/12/16.
//

#ifndef RABIT_RABIT_TRACKER_H
#define RABIT_RABIT_TRACKER_H

#include <assert.h>
#include <set>
#include "Socket.h"
#include "cstdio"
#include "string"
#include "pthread.h"
#include "map"

using namespace rabit::utils;
using namespace std;

const bool LOG_TO_FILE = true;
const bool LOG_TO_CONSOLE = true;

typedef void (*submitMPI)(int nslave, vector<string> workerArgs, map<string, string> workerEnv);



void log_print(const char *fmt,...) {
    va_list ap;
    va_start(ap, fmt);
    if (LOG_TO_FILE) {
        FILE* fp = fopen("tracker_log.txt","a");
        vfprintf(fp, fmt, ap);
    }

    if (LOG_TO_CONSOLE){
        vprintf(fmt,ap);
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
    SlaveEntry(TCPSocket *sock, const char *sAddr): slave(sock) {
//        hostIp = SockAddr:
        int magic = slave.recvInt();
        char err[50];
        host = gethostbyname(sAddr);
        sprintf(err, "invalid magic number=%d from %s", magic, host->h_name);
        Assert(magic == KMAGIC, err);
        slave.sendInt(KMAGIC);
        rank = slave.recvInt();
        worldSize = slave.recvInt();
        jobId = slave.recvStr();
        cmd = slave.recvStr();
    }

    int decide_rank() {
        map<string, int> jobMap;
        if (rank >= 0) {
            return rank;
        }
        if (jobId.compare("NULL") != 0 && jobMap.find(jobId) != jobMap.end()) {
            return jobMap[jobId];
        }
        return -1;
    }

    void assign_rank(int rank, map<int, SockAddr*> &waitConn, map<int, SockAddr*> & treeMap, map<int, int> & parentMap, map<int, SockAddr> &ringMap) {
        this->rank = rank;
        set<int> nnSet;
        map<int, SockAddr*>::const_iterator it = treeMap.begin();
        while (it != treeMap.end()) {
            nnSet.insert(it->first);
            ++it;
        }

        slave.sendInt(rank);
        slave.sendInt(parentMap[rank]);
    }

    ExSocket slave;

private:

    string hostIp;
    int rank;
    int worldSize;
    hostent * host;
    string jobId;
    string cmd;
};

struct LinkMap {
    map<int, vector<int> > treeMap;
    map<int, pair<int, int> > ringMap;

};

class Tracker {
public:
    Tracker(int port = 9091, int port_end = 9999, bool verbose = true, const string &hostIp = "auto") {
        sock = TCPSocket();
        sock.Create(AF_INET);
        sock.TryBindHost(port, port_end);
        sock.Listen(128);
        this->hostIp = hostIp;
        this->port = port;
        if (hostIp.compare("auto") == 0) {
            this->hostIp = "ip";
            log_print("start listen on\n");
        }
        log_print("start listen on %s:%d", SockAddr::GetHostName().c_str(), port);
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
        t["rabit_tracker_port"] = port;
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
        int nSlave = linkMap.treeMap.size();
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
        while (shutDown.size() != nSlave) {
            TCPSocket sc = sock.Accept();
            SlaveEntry se(&sc, adr.data());
            int rank = se.decide_rank();
            log_print("@tracker All of %d nodes getting started", nSlave, 2);
        }
    }

private:
    TCPSocket sock;
    string hostIp;
    int port;
    string host;
};

void submit(int nslave, vector<string> args, submitMPI sub, bool verbose, string hostIp = "auto") {
    Tracker master = Tracker(false, 9091, 9099, hostIp);
    pthread_t * td;
    master.slaveEnvs();
//    pthread_create(td, NULL, sub, NULL);
    pthread_join(*td, NULL);
}

#endif //RABIT_RABIT_TRACKER_H
