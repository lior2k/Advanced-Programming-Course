#define REDIRECT 1
#define REDIRECT_STDERR 2
#define APPEND 3

#define BACKSLASH 127

#define BUFF_SIZE 1024

typedef struct var {
    char *name;
    char *value;
} var;