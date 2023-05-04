#include <pthread.h>

typedef struct task {
    char *data;
    struct task *next;
} task_t;

typedef struct queue {
    task_t *head;
    task_t *tail;
} queue_t;

typedef struct threadpool {
    int numCPU;
    pthread_t *tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    queue_t *task_queue;
    void (*func)();
    int key;
    int shutdown;
} threadpool_t;