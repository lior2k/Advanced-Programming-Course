
To do:
Makefile.
Test edge cases / memory leaks.
Example file.

# Shell
###### Author - Lior Shachoach

#### Notes
Tried making each section as similar as possible to the real version of Bash.
- Shell variables syntax: "$name = value". ($ and spaces are crucial).
- Read: Supporting different amount of names and values.
- Arrows (up/down): Supporting editing of previous commands (deleting/appending). Though no right/left arrows.
- If/Else: Supporting any number of 'then' expressions as well as 'else' expressions, no limit.
- Piping & If/Else not supported simultaneously.

## Features

1A) Redirection to stderr:
    1) Creation of file with the specified name.
    2) Close stderr.
    3) Use dup on fd to give specified file the fd of stderr (will be stderr for sure because its the lowest one possible).
    4) Close fd.

1B) Redirection to the end of another file:
Same as above except close stdout and open the new file with append flag.

2) Change prompt:
declare prompt of len of argv[1] + 2, copy argv[1] into
prompt var and append ": " to it.

3) Echo:
Iterate argv and print each arg, if it starts with '$' look it up in the var array and print the var if exists.

4) Echo $? (print status):
Print status shifted right 8 times because the first 8 bits are saved for signals or what not.

5) Change current working directory: chdir...

6) Repeat last command ("!!"):
Since I needed to save the commands history to implement the arrows section (12), all that was needed to be done was apply a condition to copy the last saved command when "!!" is entered.

7) Quit: if "quit" break...

8) Using 'signal' method to override Sigint signal to prevent quit on 'ctrl-c'.

9) Piping:
    * Tokenize by '|' to split each command.
    * Iterate tokenized commands.
        * Tokenize each command by ' ' as usual.
        * Redirect input/output of each command using pipe/dup2 methods.

10) Shell variables:
Created a struct named 'var' which holds 2 char arrays, the first for the variable name and the other for the value. In main, hold an array of vars and whenever a new variable is entered using the '$varname = value' syntax, first search to see if a variable with such name already exists, if so change the value to the new value, otherwise, extend the array and append a new var to the end.

11) Read command:
Read 2 lines and tokenize by space as usual.
Using our saveVar function from section 10 I save each named expression to each value respectively (firstrow[i+1] -> secondrow[i]).
If there are more names then values, simply ignore the redundant names.
If there are more values then names, append the last values to the last named expression.

12) Commands history and browse using up&down arrows:
Switched the input of the program to be char by char rather then line by line.
The program listens to the input at the begining of each iteration until a '\n' is read.
Each command is saved in a 2d char array (char**) as a history/log of commands.
*Disabled Echo and Canonical Input*.
Input is handled char by char, meaning every letter entered is trigerring a realloc and extension to current input as well as a rewrite of the whole line.
Special cases are up&down arrows as well as backslash(delete).
For backslash, as long as something is written, rewrite the line without the last char and replace that char with a '\0'.
For arrows, there is an int acting as a runner on the commands history, each arrow up is incrementing the runner and arrow down is decrementing. When an arrow is read, rewrite the line to the current command the runner is pointing to. Commands that are accessible by arrow keys are also modifiable, supported with backslash(delete) as well as append of new chars. No left/right arrows though.

13) If Else stream:
Whenever a command starts with 'if', read lines until 'fi' is entered.
While reading those lines, save the index of the 'else' statement if exists.
Save 2 flags, one for the fork->exec at the continuation of the loop and the second for the following iterations. if 'ifing' (first flag) then exec argv[1] instead of argv[0].
Then check the status of the executed command and set a pointer (pcmd) to the lines read earlier, if 0 set it to 1, else set it to the 'else' index + 1 (as said before we save that index in a variable).
In the next iterations, disable IO and execute commands one by one following the pcmd set earlier.

