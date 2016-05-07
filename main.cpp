#include <iostream>
#include <sstream>
#include "rabit_tracker.h"


// https://github.com/dmlc/rabit/blob/be50e7b63224b9fb7ff94ce34df9f8752ef83043/src/allreduce_base.cc#L57

using namespace std;


char testCmd[] = "~/ClionProjects/rabit/test/basic.rabit";

void execute(char * command, char * const env[]);
void mpiSubmit(int nslave, vector<string> workerArgs, map<string, string> workerEnv);
void printHelp();

void testEnv();

string hostfile;

const char echo[] = "echo %s rabit_num_trial=$nrep;";
const char keepalive[] =
        "nrep=0\n"
        "rc=254\n"
        "while [ $rc -eq 254 ];\n"
        "do\n"
            "export rabit_num_trial=$nrep\n"
            "echo 'hellp'\n"
            "echo 'shell'\n"
            "%s\n"
            "%s\n"
            "rc=$?;\n"
            "nrep=$((nrep+1));\n"
        "done\n";

char executable[50];

int main(int argv, char * args[]) {
//    execute("");
//    system("~/ClionProjects/rabit/test/basic.rabit 2");
    execute(testCmd, NULL);
    vector<string> params;
    testEnv();

    if (argv < 2) {
        printHelp();
        exit(0);
    }

//    system(keepalive);
//    puts(keepalive);

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


void execute(char * command, char * const env[]) {
    pid_t pid = fork();
    char cmd1[100], cmd2[100];
    sprintf(cmd2, echo, command);
//    printf("");
    sprintf(cmd1, keepalive, cmd2, command);
    char *name[] = {
            (char *) "/bin/bash",
            (char *) "-c",
            cmd1,
            NULL
    };
    printf(name[2]);
//    execve(name[0], name, NULL);
    switch (pid) {
        case -1:
            fprintf(stderr, "fork() failed.\n");
        case 0: // child process
            execve(name[0], name, env);
            perror("execve");
            exit(EXIT_FAILURE);
        default:
            debug_print("Create new Process with Pid %d to exec %s\n", pid, executable);
            int status = 1;

            while (!WIFEXITED(status)) {
                waitpid(pid, &status, 0); /* Wait for the process to complete */
            }

            debug_print("Process exited with %d\n", WEXITSTATUS(status));
    }
}
