#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#include <termio.h>

#define REDIRECT 1
#define REDIRECT_STDERR 2
#define APPEND 3

#define BACKSLASH 127

#define BUFF_SIZE 1024

int n, commandsSize;

typedef struct var {
    char *name;
    char *value;
} var;

/* return pointer to var if exists, null otherwise. */
var* lookup(var *variables, char *named) {
    int i = 0;
    while (1) {
        if (i >= n) {
            break;
        }
        if (!strcmp(variables[i].name, named)) {
            return variables + i;
        }
        i++;
    }
    return NULL;
}

void handle_sigint(int sig) {
    printf("you typed Control-C!\n");
}

int getch() {
   struct termios oldtc;
   struct termios newtc;
   int ch;
   tcgetattr(STDIN_FILENO, &oldtc);
   newtc = oldtc;
   newtc.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newtc);
   ch=getchar();
   tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
   return ch;
}

void reWrite(char *prompt, char *command) {
    printf("\x1b[2K"); //delete line
    printf("\r"); // move cursor to the begining of the line
    if (command != NULL) {
        printf("%s%s", prompt, command); // rewrite the line without the last char
    } else {
        printf("%s", prompt);
    }
}

char* IOControl(char *prompt, char *command, char **commands) {
    int k = 0, ch = 0, runner = -1;
    while (1) {    
        ch = getch();
        if (ch == 27) {
            ch = getch(); // read the [[
            ch = getch(); // real value - should be A / B
            if (ch == 'A') {
                // printf("Arrow Up\n");
                if (commandsSize >= 1 && runner < commandsSize - 1) {
                    runner++;
                    strcpy(command, commands[runner]);
                    k = strlen(command);
                    reWrite(prompt, command);
                }
            } else if (ch == 'B') {
                // printf("Arrow Down\n");
                if (runner > 0) {
                    runner--;
                    strcpy(command, commands[runner]);
                    k = strlen(command);
                    reWrite(prompt, command);
                } else if (runner == 0) {
                    runner--;
                    k = 0;
                    reWrite(prompt, NULL);
                }
            }
            // else {
            //     printf("Arrow Left/Right\n");
            // }
        } else {
            putchar(ch);
            command[k++] = ch;
            if (ch == '\n') {
                command[k-1] = '\0';
                break;
            } else if (ch == BACKSLASH) {
                k--;
                if (k > 0) {
                    command[--k] = '\0'; // delete last char
                    reWrite(prompt, command);
                }    
            }
        }
    }
    return command;
}

char* changePrompt(char *prompt, char *newPrompt) {
    prompt = (char *) malloc(sizeof(char)*strlen(newPrompt));
    strcpy(prompt, newPrompt);
    return prompt;
}

void echo(char **argv, int status, var *var_array) {
    if (!strcmp(argv[1], "$?")) {
        printf("%d", status >> 8);
    } else {
        int j = 1;
        while (argv[j]) {
            char* curr = argv[j];
            if (curr[0] == '$') {
                var *variable = lookup(var_array, ++curr); //++curr to skip the $ sign because for example $x is saved as x
                if (variable != NULL)
                    printf("%s ", variable->value);
            } else {
                printf("%s ", curr);
            }
            j++;
        }
    }
    printf("\n");
}

var* saveVar(char **argv, var *var_array) {
    char *varName = argv[0];
    varName++;
    char *varVal = argv[2];
    var *exist = lookup(var_array, varName);
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
    return var_array;
}

void saveCommandInHistory(char **commands, char *command) {
    commandsSize++;
    commands = (char**)realloc(commands, commandsSize*sizeof(char*));
    commands[commandsSize - 1] = malloc(strlen(command)*sizeof(char));
    strcpy(commands[commandsSize - 1], command);
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

    var *var_array = (var*) malloc(0);
    n = 0; // size of var array

    char **commands = (char **) malloc(sizeof(char*));
    commandsSize = 0; // size of commands array

    while(1) {
        printf("%s", prompt);

        command = IOControl(prompt, command, commands);

        /* note - if command equals to "!!" then change it to the previous command
         and execute the previous command, otherwise, save the new command in var
         prevCommand and execute the new command regularly */
        if (!strcmp(command, "!!\0")) {
            command = calloc(BUFF_SIZE, sizeof(char));
            strcpy(command, prevCommand);
        } else {
            strcpy(prevCommand, command);
        }

        if (command != NULL)
            saveCommandInHistory(commands, command);

        /* parse command line */
        i = 0;
        token = strtok(command," ");
        while (token != NULL) {
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

        if (!strcmp(argv[0], "prompt") && !strcmp(argv[1], "=")) {
            prompt = changePrompt(prompt, argv[2]);
            continue;
        }

        if (!strcmp(argv[0], "echo")) {
            echo(argv, status, var_array);
            continue;
        }

        if (!strcmp(argv[0], "cd")) {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        if (argv[0][0] == '$' && !strcmp(argv[1], "=")) {
            var_array = saveVar(argv, var_array);
            continue;
        }

        /* Does command line end with & */ 
        if (!strcmp(argv[i - 1], "&")) {
            amper = 1;
            argv[i - 1] = NULL;
        } else 
            amper = 0;
        
        if (i >= 2 && (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], "2>") || !strcmp(argv[i - 2], ">>"))) {
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
            signal(SIGINT, SIG_DFL); // remove the ctrl c signal
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
            return 1; // file was not found - killing the child process by returning
        }  
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);

    }

    return 0;
}