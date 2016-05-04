
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

// exec替换当前的进程，所以要结合fork使用

void excute(char * const command[], char * const env[]);
char executable[50];

int
main(int argc, char *argv[])
{
    char * newargv[] = { const_cast<char*>("~/ClionProjects/rabit/test/"), NULL };
    char *newenviron[] = {const_cast<char*>("env-key=23123") };

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file-to-exec>\n", argv[0]);
//        exit(EXIT_FAILURE);
        char d[] = "/Users/fripside/ClionProjects/rabit/test/env";
        stpcpy(executable, d);
    } else {
        stpcpy(executable, argv[1]);
    }

    newargv[0] = argv[1];
    puts(argv[1]);
    char str[20];
    std::string val;
    for (int i = 0; i < 5; ++i) {
        val = ('0' + i);
        strcpy(str, val.data());
        newargv[0] = str;
        printf("Round: %s\n", val.data());
//        execve(executable, newargv, newenviron);
//        perror("execve");   /* execve() returns only on error */
        excute(newargv, newenviron);

    }
    exit(EXIT_FAILURE);

}

void excute(char * const command[], char * const env[]) {
    pid_t pid = fork();
    switch (pid) {
        case -1:
            fprintf(stderr, "fork() failed.\n");
        case 0: // child process
            execve(executable, command, env);
            perror("execve");
            exit(EXIT_FAILURE);
        default:
            printf("Create new Process with Pid %d to exec %s\n", pid, executable);
            int status;

            while (!WIFEXITED(status)) {
                waitpid(pid, &status, 0); /* Wait for the process to complete */
            }

           printf("Process exited with %d\n", WEXITSTATUS(status));
    }
}
