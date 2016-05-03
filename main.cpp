#include <iostream>
#include <sstream>
#include "rabit_tracker.h"


// https://github.com/dmlc/rabit/blob/be50e7b63224b9fb7ff94ce34df9f8752ef83043/src/allreduce_base.cc#L57

using namespace std;

void mpiSubmit(int nslave, vector<string> workerArgs, map<string, string> workerEnv);
void printHelp();

string hostfile;

int main(int argv, char * args[]) {
    vector<string> params;
    if (argv < 2) {
        printHelp();
        exit(0);
    }

    int i = 1;
    int nworks = 1;
    for (int i = 0; i < argv; ++i) {

    }
    while (i < argv) {
        params.push_back(args[i]);
        string str = args[i];
        if (i + 1 < argv && (str.compare("-n") == 0 || str.compare("--nworker") == 0)) {
            nworks = atoi(args[++i]);
            params.push_back(args[i]);
        }
        if (i + 1 < argv && (str.compare("-H") == 0 || str.compare("--hostfile") == 0)) {
            hostfile = atoi(args[++i]);
            params.push_back(args[i]);
        }

        ++i;
    }
//    cout << "nwroks " << nworks << endl;
    submit(nworks, params, mpiSubmit, true, "auto");
    return 0;
}

void mpiSubmit(int nslave, vector<string> workerArgs, map<string, string> workerEnv) {
    for (auto it = workerEnv.begin(); it != workerEnv.end(); ++it) {
        workerArgs.push_back(it->first + "=" + it->second);
    }
    stringstream ss;
    ss << "mpirun -n ";
    ss << nslave;
    if (!hostfile.empty()) {
        ss << " --hostfile ";
        ss << hostfile;
    }
    for (int i = 0; i < workerArgs.size(); ++i) {
        ss << " " << workerArgs[i];
    }
    string cmd;
    ss >> cmd;
    system(cmd.c_str());
}

void printHelp() {
    cout << "Usage:" << endl;
    cout << "Required: -n " << "--nworker        " << "number of worker proccess to be launched" << endl;
    cout << "Optional: -v " << "--verbose [0, 1] " << "print more messages into the console" << endl;
    cout << "Optional: -H " << "--hostfile       " << "the hostfile of mpi server" << endl;
    cout << "Optional: command " << "[c1] [c2]   " << "command for rabit program" << endl;
}
