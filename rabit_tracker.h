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
#include <stdarg.h>
#include "string"
#include "pthread.h"
#include "map"
#include "time.h"

using namespace rabit::utils;
using namespace std;
using std::map;

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
    /*va_list ap;
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
    va_end(ap);*/
}


const int KMAGIC = 0xff99;

class ExSocket {
public:
    ExSocket(TCPSocket s): sock(s) {
    }

    int sendInt(int t) {
        size_t s = sock.SendAll(&t, sizeof t);
        printf("sendInt %d %lu\n", t, s);
        return (int) s;
    }

    int sendStr(string str) {
        char const * s = str.c_str();
        size_t t = strlen(s);
        sendInt((int) t);
        sock.SendAll(s, t);
        printf("sendStr %s %lu\n", str.data(), t);
        return (int) t;
    }

    int recvInt() {
        int t;
        sock.RecvAll(&t, sizeof t);
        return t;
    }

    string recvStr() {
        int t = recvInt();
        char s[50];
        sock.RecvAll(s, t);
        s[t] = 0;
        return string(s);
    }

private:
    TCPSocket sock;

};

class SlaveEntry {
public:
    SlaveEntry(TCPSocket tcpSock): sock(tcpSock) {
//        hostIp = SockAddr:
        int magic = sock.recvInt();
        char err[50];
        host = tcpSock.sAddr.AddrStr();
        port = tcpSock.sAddr.port();
        sprintf(err, "invalid magic number=%d from %s:%d", magic, host.data(), port);
        Assert(magic == KMAGIC, err);
        sock.sendInt(KMAGIC);
        rank = sock.recvInt();
        worldSize = sock.recvInt();
        jobId = sock.recvStr();
        cmd = sock.recvStr();
        printf("Init SlaveEntry RECV: %d %d %s %s\n", rank, worldSize, jobId.data(), cmd.data());
    }

    int decide_rank(map<string, int> & jobMap) {
        printf("Decide Rank: %d\n", rank);
        if (rank >= 0) {
            return rank;
        }
        if (jobId.compare("NULL") != 0 && jobMap.find(jobId) != jobMap.end()) {
            return jobMap[jobId];
        }
        return -1;
    }

    void dumpSet(string msg, set<int> &s) {
        printf("============%s===========\n", msg.data());
        for (set<int>::iterator sit = s.begin(); sit != s.end(); ++sit) {
            printf("%d ", *sit);
        }
        printf("\n");
    }

    vector<int> assign_rank(int rank, map<int, SlaveEntry*> &waitConn, map<int, vector<int> > &treeMap, map<int, int> & parentMap, map<int, pair<int, int> > &ringMap) {
        this->rank = rank;
        set<int> nnSet = set<int>(treeMap[rank].begin(), treeMap[rank].end());
        int rprev = ringMap[rank].first, rnext = ringMap[rank].second;
        printf("Start Assign Rank: %d %d:%d\n", rank, rprev, rnext);
        printf("send %d %lu %lu\n" ,parentMap[rank], treeMap.size(), nnSet.size());
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
            set_difference(nnSet.begin(), nnSet.end(), goodSet.begin(), goodSet.end(), inserter(badSet, badSet.begin()));

            dumpSet("nnSet", nnSet);
            dumpSet("goodSet", goodSet);
            dumpSet("badSet", badSet);

            vector<int> conSet;
            for (set<int>::iterator iter = badSet.begin(); iter != badSet.end(); ++iter) {
                map<int, SlaveEntry*>::iterator wit = waitConn.find(*iter);
                if (wit != waitConn.end()) {
                    conSet.push_back(*iter);
                }
            }
            printf("conSet Size: %lu", conSet.size());
            sock.sendInt((int) conSet.size());
            int left = ((int) badSet.size()) - (int) conSet.size();
            sock.sendInt(left);

            for (int i = 0; i < conSet.size(); ++i) {
                printf("send conSet:%d  %s:%d\n", conSet[i], waitConn[conSet[i]]->host.data(), waitConn[conSet[i]]->port);
                sock.sendStr(waitConn[conSet[i]]->host);
                sock.sendInt(waitConn[conSet[i]]->port);
                sock.sendInt(conSet[i]);
            }
            int nErr = sock.recvInt();
            if (nErr != 0) {
                printf("ERROR:  %d\n", nErr);
                continue;
            }
            int p = sock.recvInt();
            printf("renew port old:%d new:%d\n", port, p);
            port = p;
            vector<int> rmSet;
            for (int i = 0; i < conSet.size(); ++i) {
                int r = conSet[i];
                waitConn[r]->waitAccept -= 1;
                if (waitConn[r]->waitAccept == 0) rmSet.push_back(r);
            }
            for (int i = 0; i < rmSet.size(); ++i) {
                map<int, SlaveEntry*>::iterator iter = waitConn.find(rmSet[i]);
                waitConn.erase(iter);
                //delete iter->second;
            }
            waitAccept = ((int) badSet.size()) - (int) conSet.size();
            return rmSet;
        }
    }

    ExSocket sock;

public:
    int port;
    string hostIp;
    int rank;
    int worldSize;
    string host;
    string jobId;
    string cmd;
    int waitAccept;
};

struct LinkMap {
    map<int, vector<int> > treeMap;
    map<int, pair<int, int> > ringMap;
    map<int, int> parentMap;
    void dump() {
        printf("treeMap %lu\n", treeMap.size());
        // 0: [1, 2], 1: [0, 3, 4], 2: [0], 3: [1], 4: [1]
        for (int i = 0; i < treeMap.size(); ++i) {
            printf("%d :[", i);
            for (int j = 0; j < treeMap[i].size(); ++j) {
                printf("%d, ", treeMap[i][j]);
            }
            printf("], ");
        }
        printf("\n");

        printf("ringMap %lu\n", ringMap.size());
        map<int, pair<int, int> >::iterator rit = ringMap.begin();
        while (rit != ringMap.end()) {
            printf("%lu: %d(%d, %d), ", ringMap.size(), rit->first, rit->second.first, rit->second.second);
            ++rit;
        }
        printf("\n");

        printf("parentMap %lu\n", parentMap.size());
        //0: -1, 1: 0, 2: 0, 3: 1, 4: 1
        for (int i = 0; i < parentMap.size(); ++i) {
            printf("%d: %d, ", i, parentMap[i]);
        }
        printf("\n");
    }
};

bool cmpSlave(SlaveEntry *s1, SlaveEntry *s2) { //<
    return s1->host.compare(s2->host) < 0;
}

class Tracker {
public:
    Tracker(string url, int start_port = 9091, int end_port = 9999, bool verbose = true, const string &hostIp = "auto"): port(-1) {
        TCPSocket sock;
        sock.Create();
        sockfd = sock.sockfd;

        SockAddr addr;
        for (int p = start_port; p < end_port; ++p) {
            addr = SockAddr(url.data(), p);
            if (::bind(sockfd, reinterpret_cast<const sockaddr*>(&addr.addr),
                       sizeof(addr.addr)) == 0) {
                port = p;
                break;
            } else {
                printf("Failed to Bind %s:%d\n", addr.AddrStr().data(), addr.port());
            }
        }
        if (port == -1) {
            sock.GetSockError();
            sock.GetLastError();
            printf("Failed to Bind %s:%d\n", addr.AddrStr().data(), addr.port());
            exit(-1);
        }

        sock.Listen();
        printf("Start to Listen %s:%d\n", addr.AddrStr().data(), addr.port());
        this->hostIp = hostIp;
        host = addr.AddrStr();
    }

    ~Tracker() {
        TCPSocket sock(sockfd);
        sock.Close();
    }

    map<string, string> slaveEnvs() {
        map<string, string> t;
        printf("%s %s\n", hostIp.data(), host.data());
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

    LinkMap getTree(int nSlave) {
        LinkMap linkMap;
        for (int i = 0; i < nSlave; ++i) {
            linkMap.treeMap[i] = getNeighbor(i, nSlave);
            linkMap.parentMap[i] = (i + 1) / 2 - 1;
        }
        return linkMap;
    }

    vector<int> findShareRing(LinkMap &linkMap, int r) {
        set<int> nSet(linkMap.treeMap[r].begin(), linkMap.treeMap[r].end());
        set<int> parentSet;
        parentSet.insert(linkMap.parentMap[r]);
        set<int> cSet;
        set_difference(nSet.begin(), nSet.end(), parentSet.begin(), parentSet.end(), inserter(cSet, cSet.begin()));
        vector<int> rlst;
        rlst.push_back(r);
        if (cSet.size() == 0) {
            return rlst;
        }
        int cnt = 0;
        set<int>::iterator iter = cSet.begin();
        while (iter != cSet.end()) {
            vector<int> vlst = findShareRing(linkMap, *iter);
            cnt += 1;
            if (cnt == cSet.size()) {
                reverse(vlst.begin(), vlst.end());
            }
            rlst.insert(rlst.end(), vlst.begin(), vlst.end());
            ++iter;
        }

        return rlst;
    }

    map<int, pair<int, int> > getRing(LinkMap &linkMap) {
        map<int, pair<int, int> > ringMap;
        int nSlave = (int) linkMap.treeMap.size();
        vector<int> rlst = findShareRing(linkMap, 0);
        printf("ASSET getRing %d=%d", (int) rlst.size(), (int) linkMap.treeMap.size());
        for (int i = 0; i < nSlave; ++i) {
            int rprev = (i + nSlave - 1) % nSlave;
            int rnext = (i + 1) % nSlave;
            ringMap[rlst[i]] = pair<int, int>(rlst[rprev], rlst[rnext]);
        }
        return ringMap;
    }

    LinkMap getLinkMap(int nSlave) {
        LinkMap linkMap = getTree(nSlave);
        linkMap.ringMap = getRing(linkMap);
        printf("---------------\nLinkMap Raw\n");
        linkMap.dump();
        map<int, int> rmap;
        int k = 0;
        for (int i = 0; i < nSlave - 1; ++i) {
            k = linkMap.ringMap[k].second;
            rmap[k] = i + 1;
        }

        LinkMap lMap;
        for (map<int, pair<int, int> >::iterator rit = linkMap.ringMap.begin(); rit != linkMap.ringMap.end(); ++rit) {
            pair<int, int> &val = rit->second;
            lMap.ringMap[rmap[rit->first]] = pair<int, int>(rmap[val.first], rmap[val.second]);
        }
        for (map<int, vector<int> >::iterator tit = linkMap.treeMap.begin(); tit != linkMap.treeMap.end(); ++tit) {
            vector<int> &val = tit->second;
            for (int i = 0; i < val.size(); ++i) {
                lMap.treeMap[rmap[tit->first]].push_back(rmap[val[i]]);
            }
        }
        for (map<int, int>::iterator pit = linkMap.parentMap.begin(); pit != linkMap.parentMap.end(); ++pit) {
            if (pit->first != 0) {
                lMap.parentMap[rmap[pit->first]] = rmap[pit->second];
            } else {
                lMap.parentMap[rmap[pit->first]] = -1;
            }
        }
        printf("---------------\nLinkMap After\n");
        lMap.dump();
        return lMap;
    }

    void dumpCon(map<int, SlaveEntry*> & waitConn) {
        map<int, SlaveEntry*>::iterator wit = waitConn.begin();
        printf("\ndump waitConn: %lu\n", waitConn.size());
        for (; wit != waitConn.end(); ++wit) {
            printf("%d jobId:%s %s:%d\n", wit->first, wit->second->jobId.data(), wit->second->host.data(), wit->second->port);
        }
    }

    void dumpPending(vector<SlaveEntry *> &pending) {
        vector<SlaveEntry *>::iterator pit = pending.begin();
        printf("\ndump pending\n");
        for (; pit != pending.end(); ++pit) {
            printf("jobId:%s %s:%d\n", (*pit)->jobId.data(), (*pit)->host.data(), (*pit)->port);
        }
    }

    void acceptSlaves(int nSlave) {
        map<int, SlaveEntry*> shutDown;
        string adr("");
        map<string, int> jobMap;
        TCPSocket sock(sockfd);
        map<int, vector<int> > treeMap;
        map<int, int> parentMap;
        map<int, pair<int, int> > ringMap;
        vector<int> todoNodes;
        vector<SlaveEntry*> pending;
        map<int, SlaveEntry*> waitConn;
        while (shutDown.size() != nSlave) {
            printf("============\nStart Accept...\n==========\n");
            TCPSocket sc = sock.Accept();
            printf("Accept suc\n");
            printf("Accept Client %s:%d\n", sc.sAddr.AddrStr().data(), sc.sAddr.port());
            SlaveEntry* se = new SlaveEntry(sc);
            printf("Init Finish:%d %s\n", se->cmd.compare("print"), se->cmd.data());
            if (se->cmd.compare("print") == 0) {
                string  msg = se->sock.recvStr();
                printf("Message from %s:%d : %s\n", se->host.data(), se->port, msg.data());
            } else if (se->cmd.compare("shutdown") == 0) {
                shutDown[se->rank] = se;
                continue;
            }
            printf("ASSERT: CMD Should be [start] or [recover]: %s\n", se->cmd.data());

            if (treeMap.size() == 0) {
                printf("ASSERT: CMD Should be [start]: %s\n", se->cmd.data());
                if (se->worldSize > 0) {
                    nSlave = se->worldSize;
                }
                printf("Before NSalve %d\n", nSlave);
                LinkMap linkMap = getLinkMap(nSlave);
                printf("NSalve %d\n", nSlave);
                treeMap = linkMap.treeMap;
                parentMap = linkMap.parentMap;
                ringMap = linkMap.ringMap;
                for (int i = 0; i < nSlave; ++i) todoNodes.push_back(nSlave - i - 1);
            } else {
                printf("ASSERT: World SIZE: %d == nSlave: %d\n", se->worldSize, nSlave);
            }

            if (se->cmd.compare("recover") == 0) {
                printf("ASSERT: rank = %d > 0\n", se->rank);
            }
            //printf("Get Link Map %d\n", treeMap.size());
            int rank = se->decide_rank(jobMap);
            printf("After Decide rank:%d\n", rank);
            if (rank == -1) {
                pending.push_back(se);
                if (pending.size() == todoNodes.size()) {
                    printf("pending %lu todoNodes %lu\n", pending.size(), todoNodes.size());
                    sort(pending.begin(), pending.end(), cmpSlave);
                    dumpPending(pending);
                    for (int i = 0; i < pending.size(); ++i) {
                        SlaveEntry* s = pending[i];
                        int rank = todoNodes.back();
                        todoNodes.pop_back();
                        if (s->jobId.compare("NULL") != 0) {
                            jobMap[s->jobId] = rank;
                        }
                        //printf("Start Assign Ranka\n");
                        dumpCon(waitConn);
                        s->assign_rank(rank, waitConn, treeMap, parentMap, ringMap);
                        printf("After Assign Rank: %d\n", rank);
                        if (s->waitAccept > 0) {
                            printf("Accept: %d Put To WaitConn: rank: %d jobId:%s %s:%d\n", s->waitAccept, rank, s->jobId.data(), s->host.data(), s->port);
                            waitConn[rank] = s;
                        }
                        printf("Recieve %s signal from %s; assign rank %d\n", se->cmd.data(), se->host.data(), se->rank);
                    }
                }
                if (todoNodes.size() == 0) {
                    printf("@tracker All of %d nodes getting started\n", nSlave);
                    startTime = clock();
                }
            } else {
                printf("Before Assign: %d\n", rank);
                se->assign_rank(rank, waitConn, treeMap, parentMap, ringMap);
                printf("Else Recieve %s signal from %d\n", se->cmd.data(), se->rank);
                if (se->waitAccept > 0) {
                    waitConn[rank] = se;
                }
            }
        }
        log_print("@tracker All nodes finishes job");
        clock_t endTime = clock();
        double spend = (endTime - startTime) / (double) CLOCKS_PER_SEC;
        log_print("@tracker %.3f secs between node start and job finish", spend);
    }

private:
    SOCKET sockfd;
    string hostIp;
    int port;
    string host;
    clock_t startTime;
};

void submit(submitArgs *args, string url, submitFunc sub, bool verbose, string hostIp = "auto") {
    Tracker master(url, 9091, 9999, verbose, hostIp);
    printf("Submit...");
    pthread_t td;
    args->workerEnv =  master.slaveEnvs();
    args->workerEnv["env-key"] = "test-key";
    pthread_create(&td, NULL, sub, args);
    master.acceptSlaves(4);
    pthread_join(td, NULL);
}

#endif //RABIT_RABIT_TRACKER_H
