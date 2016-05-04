#include <iostream>
#include <sstream>
#include "rabit_tracker.h"


// https://github.com/dmlc/rabit/blob/be50e7b63224b9fb7ff94ce34df9f8752ef83043/src/allreduce_base.cc#L57

using namespace std;


const char * testCmd = "~/ClionProjects/rabit/test/basic.rabit 3";

void execute(char * const command[], char * const env[]);
void mpiSubmit(int nslave, vector<string> workerArgs, map<string, string> workerEnv);
void printHelp();

void testEnv();

string hostfile;


int main(int argv, char * args[]) {
//    execute("");
//    system("~/ClionProjects/rabit/test/basic.rabit 2");
    execute(testCmd);
    vector<string> params;
    testEnv();

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

void testEnv() {
//    setenv("TEST", "2345", 0);
    execl("./test/basic.rabit 2", "");
    const char *value = getenv("TEST");
    printf("ENV=====> %s\n", value);
}

void printHelp() {
    cout << "Usage:" << endl;
    cout << "Required: -n " << "--nworker        " << "number of worker proccess to be launched" << endl;
    cout << "Optional: -v " << "--verbose [0, 1] " << "print more messages into the console" << endl;
    cout << "Optional: -H " << "--hostfile       " << "the hostfile of mpi server" << endl;
    cout << "Optional: command " << "[c1] [c2]   " << "command for rabit program" << endl;
}


int execute(char * const command) {
    pid_t cpid;

    cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork");
            break;

        case 0:
            execl(testCmd, "", command, NULL); /* this is the child */
            _exit(EXIT_FAILURE);

        default:
            waitpid(cpid, NULL, 0);

    }
}