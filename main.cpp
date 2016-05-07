#include <iostream>
#include <sstream>
#include "rabit_tracker.h"


// https://github.com/dmlc/rabit/blob/be50e7b63224b9fb7ff94ce34df9f8752ef83043/src/allreduce_base.cc#L57

using namespace std;


char testCmd[] = "~/ClionProjects/rabit/";

void* execute(void *args);
void* mpiSubmit(void *args);
void* demoSubmit(void *args);
void printHelp();

void testEnv();

string hostfile;
string url = "127.0.0.1";
bool runMpi = false;

const char echo[] = "echo `%s rabit_num_trial=$nrep`;";
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

char executable[100];

int main(int argv, char * args[]) {
//    execute("");
//    system("~/ClionProjects/rabit/test/basic.rabit 2");
//    execute(testCmd, NULL);
    vector<string> params;
    testEnv();

    if (argv < 2) {
        printHelp();
        exit(0);
    }

    // 解析命令行参数，除了help中的参数，其他的都放一起传入到exe中（basic.rabit）
    int nworks = 1;
    bool verbose = false;
//    strcat(executable, testCmd);
    for (int i = 1; i < argv; ++i) {
        printf("%s\n", args[i]);
        if (i + 1 < argv) {
            if (!strcmp(args[i], "-n") || !strcmp(args[i], "--nworker")) {
                nworks = atoi(args[++i]);
            } else if (!strcmp(args[i], "-h") || !strcmp(args[i], "--hostfile")) {
                hostfile = args[++i];
            } else if (!strcmp(args[i], "-v") || !strcmp(args[i], "--verbose")) {
                verbose = atoi(args[++i]) == 1;
            } else if (!strcmp(args[i], "-u") || !strcmp(args[i], "--url")) {
                url = args[++i];
            } else if (!strcmp(args[i], "-r") || !strcmp(args[i], "--run")) {
                if (!strcmp(args[++i], "mpi")) runMpi = true;
            } else {
                strcat(executable, args[i]);
                strcat(executable, " ");
            }
        } else {
            strcat(executable, args[i]);
            strcat(executable, " ");
        }
    }
    printf("%d %s %d %s\n", nworks, url.data(), verbose, executable);
    submitArgs submitA(nworks);
    submitA.cmd = executable;
    if (runMpi) {
        submit(&submitA, url, mpiSubmit, true, "auto");
    } else {
        submit(&submitA, url, demoSubmit, true, "auto");
    }

    return 0;
}

void* mpiSubmit(void *args) {
    submitArgs* submitA = reinterpret_cast<submitArgs*>(args);
    for (auto it = submitA->workerEnv.begin(); it != submitA->workerEnv.end(); ++it) {
        submitA->workerArgs.push_back(it->first + "=" + it->second);
    }
    printf("mpiSubmit");
    stringstream ss;
    ss << "mpirun -n ";
    ss << submitA->nSlave;
    if (!hostfile.empty()) {
        ss << " --hostfile ";
        ss << hostfile;
    }
    for (int i = 0; i < submitA->workerArgs.size(); ++i) {
        ss << " " << submitA->workerArgs[i];
    }
    string cmd;
    ss >> cmd;
    system(cmd.c_str());

    return NULL;
}

void* demoSubmit(void *args) {
    printf("\n run demoSubmit\n");
    submitArgs* submitA = reinterpret_cast<submitArgs*>(args);
    submitA->cmd = executable;
    pthread_t pth;
    pthread_create(&pth, NULL, execute, submitA);
    pthread_join(pth, NULL);
    return NULL;
}

void testEnv() {
    setenv("TEST", "2345", 0);
//    setenv("TEST")
//    execl("./test/basic.rabit 2", "");
    const char *value = getenv("TEST");
    printf("ENV=====> %s\n", value);
}

void printHelp() {
    cout << "Usage:" << endl;
    cout << "Optional: -u " << "--url" << "tracker ip";
    cout << "Required: -n " << "--nworker        " << "number of worker proccess to be launched" << endl;
    cout << "Optional: -v " << "--verbose [0, 1] " << "print more messages into the console" << endl;
    cout << "Optional: -H " << "--hostfile       " << "the hostfile of mpi server" << endl;
    cout << "Optional: -r " << "--run mpi,demo   " << "run mpi or demo (default demo)" << endl;
    cout << "Optional: command " << "[c1] [c2]   " << "command for rabit program" << endl;
}


void* execute(void *args) {
    submitArgs* submitA = reinterpret_cast<submitArgs*>(args);
    char const *command = submitA->cmd.c_str();
    char *env[20] = {NULL};
    pid_t pid = fork();
    char cmd1[100], cmd2[100];
    sprintf(cmd2, echo, command);
    sprintf(cmd1, keepalive, cmd2, command);
    char *name[] = {
            (char *) "/bin/bash",
            (char *) "-c",
            cmd1,
            NULL
    };
    printf(cmd1);
//    execve(name[0], name, NULL);
    switch (pid) {
        case -1:
            fprintf(stderr, "fork() failed.\n");
        case 0: // child process
            execve(name[0], name, NULL);
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
