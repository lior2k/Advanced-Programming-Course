#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

#define REDIRECT 1
#define REDIRECT_STDERR 2
#define APPEND 3

void lookupAndPrint(char** names, char** values, char *named) {
    int i = 0;
    while (names[i]) {
        if (!strcmp(names[i], named)) {
            printf("%s ", values[i]);
            break;
        }
        i++;
    }
}

int main() {
    int i, fd, amper, retid, status, redirect;
    char *outfile;
    char *argv[10];
    char command[1024];
    char *token;

    char *prompt = "hello: ";
    char prevCommand[1024];
    char *varNames[20];
    char *varVals[20];

    while(1) {
        strcpy(prevCommand, command);

        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0'; // replace \n with \0

        if (!strcmp(command, "!!\0")) {
            strcpy(command, prevCommand);
            
        }

        /* parse command line */
        i = 0;
        token = strtok (command," ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok (NULL, " ");
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
            prompt = (char *) malloc(sizeof(char)*strlen(argv[2]) + 2);
            strcpy(prompt, argv[2]);
            prompt[strlen(prompt)] = ':';
            prompt[strlen(prompt)] = ' ';
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
                        // lookupAndPrint(varNames, varVals, curr);
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
        }  
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
        else
            amper = 0;
    }

    return 0;
}