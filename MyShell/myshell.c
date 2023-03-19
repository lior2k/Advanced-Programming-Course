#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>

#define REDIRECT 1
#define REDIRECT_STDERR 2
#define APPEND 3

#define BUFF_SIZE 1024

/* return the index of the variable name in the names array, the same index
can be used to extract the value from the val array. -1 if not found. */
int lookup(char** names, int n, char *named) {
    int i = 0;
    while (1) {
        if (i >= n) {
            break;
        }
        if (!strcmp(names[i], named)) {
            return i;
        }
        i++;
    }
    return -1;
}

void handle_sigint(int sig) {
    printf("\nyou typed Control-C!");
}

int main() {
    signal(SIGINT, handle_sigint);
    int i, fd, amper, retid, status, redirect;
    char *outfile;
    char *argv[10];
    char *command = (char*) malloc(sizeof(char)*BUFF_SIZE);
    char *token;

    char *prompt = "hello: ";
    char *prevCommand = (char*) malloc(sizeof(char)*BUFF_SIZE);
    char **varNames;
    char **varVals;
    int n = 0; // size of var array

    while(1) {
        printf("%s", prompt);
        fgets(command, BUFF_SIZE, stdin);
        command[strlen(command) - 1] = '\0'; // replace \n with \0

        /* note - if command equals to "!!" then change it to the previous command
         and execute the previous command, otherwise, save the new command in var
         prevCommand and execute the new command regularly */
        if (!strcmp(command, "!!\0")) {
            command = calloc(BUFF_SIZE, sizeof(char));
            strcpy(command, prevCommand);
        } else {
            strcpy(prevCommand, command);
        }

        /* parse command line */
        i = 0;
        token = strtok(command," ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        argv[i] = NULL;

        /* Is command empty */ 
        if (argv[0] == NULL)
            continue;

        if (!strcmp(command, "quit")) {
            break;
        }

        // for built in shell commands
        if (!strcmp(argv[0], "prompt") && !strcmp(argv[1], "=")) {
            // prompt = (char *) malloc(sizeof(char)*strlen(argv[2]) + 2);
            prompt = (char *) malloc(sizeof(char)*strlen(argv[2]));
            strcpy(prompt, argv[2]);
            // prompt[strlen(prompt)] = ':';
            // prompt[strlen(prompt)] = ' ';
            continue;
        }

        if (!strcmp(argv[0], "echo")) {
            if (!strcmp(argv[1], "$?")) {
                printf("%d", status);
            } else {
                int j = 1;
                while (argv[j]) {
                    char* curr = argv[j];
                    if (curr[0] == '$') {
                        int index = lookup(varNames, n, ++curr);
                        if (index != -1)
                            printf("%s ", varVals[index]);
                    } else {
                        printf("%s ", curr);
                    }
                    j++;
                }
            }
            printf("\n");
            continue;
        }

        if (!strcmp(argv[0], "cd")) {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        if (argv[0][0] == '$' && !strcmp(argv[1], "=")) {
            char *varName = argv[0];
            varName++;
            char *varVal = argv[2];
            int index = lookup(varNames, n, varName);
            if (index != -1) { // var already exists -> change the value
                varVals[index] = calloc(strlen(varVal), sizeof(char));
                strcpy(varVals[index], varVal);
            } else { // var doesn't exist -> add new value
                n++;
                varNames = realloc(varNames, n * sizeof(char*));
                varVals = realloc(varVals, n * sizeof(char*));
                varNames[n-1] = malloc(sizeof(char)*strlen(varName));
                varVals[n-1] = malloc(sizeof(char)*strlen(varVal));
                strcpy(varNames[n-1], varName);
                strcpy(varVals[n-1], varVal);
            }
            continue;
        }

        /* Does command line end with & */ 
        if (!strcmp(argv[i - 1], "&")) {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else 
            amper = 0;
        
        if (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], "2>") || !strcmp(argv[i - 2], ">>")) {
            if (!strcmp(argv[i - 2], ">")) {
                redirect = REDIRECT;
            } else if (!strcmp(argv[i - 2], "2>")) {
                redirect = REDIRECT_STDERR;
            } else {
                redirect = APPEND;
            }
            outfile = argv[i - 1];
            argv[i - 2] = NULL;
        } else {
            redirect = 0;
        }
        /* for commands not part of the shell command language */ 
        if (fork() == 0) { // only the child process enters this if, the parent doesn't
            /* redirection of IO ? */
            if (redirect) {
                if (redirect == REDIRECT || redirect == REDIRECT_STDERR) {
                    fd = creat(outfile, 0660);
                } else {
                    fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                }
                
                if (redirect == REDIRECT || redirect == APPEND) {
                    close(STDOUT_FILENO);
                } else {
                    close(STDERR_FILENO);
                }

                dup(fd); 
                close(fd);
            }
            execvp(argv[0], argv);
            return 0;
        }  
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
        else
            amper = 0;
    }

    return 0;
}