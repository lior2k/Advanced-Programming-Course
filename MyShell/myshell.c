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
#include "myshell.h"

int num_variables, num_commands;

/* return pointer to var named 'named' if exists, null otherwise. */
var* lookup(var *variables, char *named) {
    for (int i = 0; i < num_variables; i++) {
        if (!strcmp(variables[i].name, named)) {
            return variables + i;
        }
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
        printf("%s %s", prompt, command); // rewrite the line without the last char
    } else {
        printf("%s ", prompt);
    }
}

/*  Manage input from user, if arrow up/down fetch the needed command rewrite on screen,
    otherwise, process the input char or backslash  */
char* IOControl(char *prompt, char *command, char **commands) {
    int k = 0, ch = 0, runner = num_commands;
    while (1) {    
        ch = getch();
        if (ch == 27) {
            ch = getch(); // read the [[
            ch = getch(); // real value - should be A / B
            if (ch == 'A') { // arrow up
                if (runner > 0) {
                    runner--;
                    strcpy(command, commands[runner]);
                    k = strlen(command);
                    reWrite(prompt, command);
                }
            } else if (ch == 'B') { // arrow down
                if (num_commands >= 1 && runner < num_commands - 1) {
                    runner++;
                    strcpy(command, commands[runner]);
                    k = strlen(command);
                    reWrite(prompt, command);
                }  else if (runner == num_commands - 1) {
                    runner++;
                    k = 0;
                    reWrite(prompt, NULL);
                }
            }
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

void echo(char **argv, int argc, int status, var *variables) {
    if (argc >= 1 && !strcmp(argv[1], "$?")) {
        printf("%d", status >> 8);
    } else {
        int j = 1;
        while (argv[j]) {
            char* curr = argv[j];
            if (curr[0] == '$') {
                var *variable = lookup(variables, ++curr); //++curr to skip the $ sign because for example $x is saved as x
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

/*  Utilize lookup method to check if var named 'varName' already exists, if so update the value,
    otherwise, extend the var array and append the new var */
var* saveVar(char *varName, char *varVal, var *variables) {
    var *exist = lookup(variables, varName);
    if (exist != NULL) { // var already exists -> change the value
        exist->value = calloc(strlen(varVal)+1, sizeof(char));
        strcpy(exist->value, varVal);
    } else { // var doesn't exist -> add new value
        num_variables++;
        variables = realloc(variables, num_variables * sizeof(var));
        variables[num_variables-1].name = malloc(sizeof(char)*strlen(varName)+1);
        variables[num_variables-1].value = malloc(sizeof(char)*strlen(varVal)+1);
        strcpy(variables[num_variables-1].name, varName);
        strcpy(variables[num_variables-1].value, varVal);
    }
    return variables;
}

/* Append a command to the commands history, used to browse older commands using the arrow keys */
char** saveCommandInHistory(char **commands, char *command) {
    num_commands++;
    commands = (char**)realloc(commands, num_commands*sizeof(char*));
    commands[num_commands - 1] = malloc(strlen(command)*sizeof(char)+1);
    strcpy(commands[num_commands - 1], command);
    return commands;
}

/* Tokenize the command by delimiter and store tokens in argv, return argc. */
int parseCommandLine(char **argv, char *command, char *delimiter) {
    int argc = 0;
    char *token = strtok(command, delimiter);
    while (token != NULL) {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    argv[argc] = NULL;
    return argc;
}

var* myread(char **argv, int argc, var *variables) {
    char values_str[BUFF_SIZE];
    fgets(values_str, BUFF_SIZE, stdin);
    values_str[strlen(values_str) - 1] = '\0';
    char *values[10];
    int num_values = parseCommandLine(values, values_str, " ");
    int runner = 0;
    /* save each name to each variable */
    while (runner < argc - 1 && runner < num_values) {
        variables = saveVar(argv[runner+1], values[runner], variables);
        runner++;
    }
    /* incase there are more values then names, append the remaining
    values to the last named variable */
    var *last_var = lookup(variables, argv[runner]);
    while (runner < num_values) {
        last_var->value = (char*)realloc(last_var->value, strlen(last_var->value) + strlen(values[runner]) + 1);
        strcat(last_var->value, " ");
        strcat(last_var->value, values[runner]);
        runner++;
    }
    return variables;
}

void myfree(char *command, char *prompt, var *variables, char **commands, char **if_inputs) {
    free(command);
    free(prompt);
    for (int i = 0; i < num_variables; i++) {
        free(variables[i].name);
        free(variables[i].value);
    }
    free(variables);
    for (int i = 0; i < num_commands; i++) {
        free(commands[i]);
    }
    free(commands);
    int i = 0;
    while (if_inputs[i] != NULL)
        free(if_inputs[i++]);
    free(if_inputs);
}

// void swapVariablesWithValues(int argc, char **argv, var *variables) {
//     for (int i = 0; i < argc; i++) {
//         if (argv[i][0]== '$') {
//             var *variable = lookup(variables, ++argv[i]);
//             if (variable != NULL) {
//                 argv[i] = variable->value;
//             }
//         }
//     }
// }

int main() {
    signal(SIGINT, handle_sigint);
    int argc, fd, amper, status, redirect, piping;
    char *outfile;
    char *argv[10];
    char *prompt = (char*)malloc(sizeof(char)*strlen("hello:")+1);
    strcpy(prompt, "hello:");
    char *command = (char*) malloc(sizeof(char)*BUFF_SIZE);
    char **commands = (char **) malloc(sizeof(char*));
    num_commands = 0; // size of commands array
    var *variables = malloc(0);
    num_variables = 0; // size of var array
    char **if_inputs = malloc(0); // [ ["then"], [then-expr]*, ["else"], [else-expr]*, ["fi"] ]
    int ifing = 0, cond = 0, else_pos = -1, pcmd = 0;

    while(1) {
        if (!cond) {
            printf("%s ", prompt);
            command = IOControl(prompt, command, commands);
        } else if (pcmd <= 0 || if_inputs[pcmd] == NULL ||
                !strcmp(if_inputs[pcmd], "fi") || !strcmp(if_inputs[pcmd], "else")) {
            cond = 0;
            else_pos = -1;
            pcmd = 0;
            continue;
        } else {
            strcpy(command, if_inputs[pcmd++]);
        }

        /*if command equals to "!!" then change it to the previous command
         and execute the previous command, otherwise, save the new command in var
         prevCommand and execute the new command regularly */
        if (!strcmp(command, "!!\0")) {
            if (num_commands > 0) {
                command = calloc(BUFF_SIZE, sizeof(char));
                strcpy(command, commands[num_commands - 1]);
            } else {
                continue;
            }
        }

        if (command != NULL)
            commands = saveCommandInHistory(commands, command);

        if (strchr(command, '|'))
            piping = 1;
        else
            piping = 0;

        /* parse command line */
        argc = parseCommandLine(argv, command, " ");

        /* Is command empty */ 
        if (argv[0] == NULL)
            continue;

        if (!strcmp(command, "quit")) {
            break;
        }

        if (!strcmp(argv[0], "prompt") && !strcmp(argv[1], "=")) {
            prompt[0] = '\0';
            for (int i = 2; i < argc; i++) {
                if (i > 2) {
                    prompt = realloc(prompt, strlen(prompt) + 2);
                    strcat(prompt, " ");
                }
                prompt = realloc(prompt, strlen(prompt) + strlen(argv[i]) + 1);
                strcat(prompt, argv[i]);    
            }
            continue;
        }

        if (!strcmp(argv[0], "echo")) {
            echo(argv, argc, status, variables);
            continue;
        }

        if (!strcmp(argv[0], "cd")) {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        if (argc == 3 && argv[0][0] == '$' && !strcmp(argv[1], "=")) {
            variables = saveVar(++argv[0], argv[2], variables);
            continue;
        }

        if (!strcmp(argv[0], "read")) {
            variables = myread(argv, argc, variables);
            continue;
        }

        for (int i = 0; i < argc; i++) {
            if (argv[i][0]== '$') {
                var *variable = lookup(variables, ++argv[i]);
                if (variable != NULL) {
                    argv[i] = variable->value;
                }
            }
        }

        if (!strcmp(argv[0], "if")) {
            int j = 0;
            while (1) {
                if_inputs = (char**)realloc(if_inputs, sizeof(char*)*(j+2));
                char *input = malloc(sizeof(char)*BUFF_SIZE);
                fgets(input, BUFF_SIZE, stdin);
                input[strlen(input) - 1] = '\0';
                if_inputs[j++] = input;
                if (!strcmp(input, "else")) {
                    else_pos = j-1;
                }
                if (!strcmp(input, "fi")) {
                    break;
                }
            }
            if_inputs[j] = NULL;
            ifing = 1;
            cond = 1;
        }

        /* Does command line end with & */ 
        if (!strcmp(argv[argc - 1], "&")) {
            amper = 1;
            argv[argc - 1] = NULL;
        } else 
            amper = 0;
        
        /* Is command redirected */
        if (argc >= 2 && (!strcmp(argv[argc - 2], ">") || !strcmp(argv[argc - 2], "2>") || !strcmp(argv[argc - 2], ">>"))) {
            if (!strcmp(argv[argc - 2], ">")) {
                redirect = REDIRECT;
            } else if (!strcmp(argv[argc - 2], "2>")) {
                redirect = REDIRECT_STDERR;
            } else {
                redirect = APPEND;
            }
            outfile = argv[argc - 1];
            argv[argc - 2] = NULL;
        } else {
            redirect = 0;
        }

        if (piping) { // basicly 'unparse' because when piping first have to parse by '|'
            command = calloc(sizeof(char), BUFF_SIZE);
            strcpy(command, commands[num_commands - 1]);
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

            if (piping) {
                char *cmds[10];
                int num_cmds = parseCommandLine(cmds, command, "|");
                int pipedes[num_cmds][2];
                for (int index = 0; index < num_cmds; index++) {
                    parseCommandLine(argv, cmds[index], " ");
                    if (index < num_cmds - 1) {
                        pipe(pipedes[index]);
                    }
                    if (fork() == 0) {
                        if (index < num_cmds - 1) { // not last command -> redirect output to pipedes[1]
                            dup2(pipedes[index][1], 1);
                            close(pipedes[index][0]);
                            close(pipedes[index][1]);
                        }
                        if (index > 0) {
                            dup2(pipedes[index - 1][0], 0); // not first command -> take input from pipedes[0] from earlier iteration
                            close(pipedes[index - 1][0]);
                            close(pipedes[index - 1][1]);
                        }
                        if (!strcmp(argv[0], "if")) {
                            execvp(argv[1], argv + 1);
                        }
                        execvp(argv[0], argv);
                        myfree(command, prompt, variables, commands, if_inputs);
                        return 1;
                    }
                    if (index > 0) { // parent and not first command -> close previous pipes
                        close(pipedes[index - 1][0]);
                        close(pipedes[index - 1][1]);
                    }
                    wait(&status);
                    if ((status >> 8) != 0) {
                        return 1;
                    }
                }
                return 0;
            }

            if (ifing) {
                execvp(argv[1], argv + 1);
            }

            execvp(argv[0], argv);
            /* error with execvp - free and return (probably wrong name given) */
            myfree(command, prompt, variables, commands, if_inputs);
            return 1;
        }  
        /* parent continues here */
        if (amper == 0)
            wait(&status);
        if (ifing) {
            if ((status >> 8) == 0) {
                pcmd = 1;
            } else {
                pcmd = else_pos + 1;
            }
        }
        ifing = 0;
    }

    myfree(command, prompt, variables, commands, if_inputs);
    return 0;
}