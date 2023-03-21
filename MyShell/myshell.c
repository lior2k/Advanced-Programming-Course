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

typedef struct var {
    char *name;
    char *value;
} var;

/* return pointer to var if exists, null otherwise. */
var* lookup(var *variables, int n, char *named) {
    int i = 0;
    while (1) {
        if (i >= n) {
            break;
        }
        if (!strcmp(variables[i].name, named)) {
            return variables + (i * sizeof(var));
        }
        i++;
    }
    return NULL;
}

void handle_sigint(int sig) {
    printf("you typed Control-C!\n");
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
    var *var_array;
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
                        var *variable = lookup(var_array, n, ++curr); //++curr to skip the $ sign because for example $x is saved as x
                        if (variable != NULL)
                            printf("%s ", variable->value);
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
            var *exist = lookup(var_array, n, varName);
            if (exist != NULL) { // var already exists -> change the value
                exist->value = calloc(strlen(varVal), sizeof(char));
                strcpy(exist->value, varVal);
            } else { // var doesn't exist -> add new value
                n++;
                var_array = realloc(var_array, n * sizeof(var));
                var_array[n-1].name = malloc(sizeof(char)*strlen(varName));
                var_array[n-1].value = malloc(sizeof(char)*strlen(varVal));
                strcpy(var_array[n-1].name, varName);
                strcpy(var_array[n-1].value, varVal);
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