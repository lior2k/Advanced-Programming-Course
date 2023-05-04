#include "codec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.h"

void enqueue(queue_t *queue, char *data) {
    task_t *new_task = malloc(sizeof(task_t));
    new_task->data = malloc(sizeof(char)*strlen(data) + 1);
    strcpy(new_task->data, data);
    new_task->next = NULL;

    if (queue->tail == NULL) {
        queue->head = new_task;
        queue->tail = new_task;
    } else {
        queue->tail->next = new_task;
        queue->tail = new_task;
    }
}

task_t* dequeue(queue_t *queue) {
    if (queue->head == NULL) {
        return NULL;
    }

    task_t *task = queue->head;
    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }

    return task;
}

// Worker threads
void *worker_thread(void *arg) {
    threadpool_t* pool = (threadpool_t*) arg;

    while (1) {
        pthread_mutex_lock(&pool->mutex);
        // printf("%tid: ld\n", pthread_self());
        while (pool->task_queue->head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }

        task_t *task = dequeue(pool->task_queue);

        pthread_mutex_unlock(&pool->mutex);

        // task.task(task.arg);
        pool->func(task->data, pool->key);
        printf("%s", task->data);
    }
    return NULL;
}

threadpool_t* init_threadpool(char **argv) {
    threadpool_t *pool;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    // Number of available CPUs on the system (logical processors)
    pool->numCPU = sysconf(_SC_NPROCESSORS_ONLN);
    // Declare number of threads
    pool->tid = (pthread_t*) malloc(sizeof(pthread_t)*pool->numCPU);
    // Declare task queue
    pool->task_queue = (queue_t*) malloc(sizeof(queue_t));
    // Set key
    pool->key = atoi(argv[1]);
    // Set function to encrypt / decrypt accordingly
    char flag = argv[2][1]; // e / d
    if (flag == 'e') {
        pool->func = encrypt; // encrypt
    } else if (flag == 'd') {
        pool->func = decrypt; // decrypt
    }
    pool->shutdown = 0;
    // Initialize pool workers
    int err;
    for (int i = 0; i < pool->numCPU; i++) {
        err = pthread_create(&pool->tid[i], NULL, &worker_thread, (void *)pool);
        if (err != 0) {
            printf("Error creating thread\n");
            exit(EXIT_FAILURE);
        }
    }
    // printf("Key: %d\nNumCPU: %d\nFlag: %c\n", pool->key, pool->numCPU, flag);
    return pool;
}

void threadpool_shutdown(threadpool_t* pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_broadcast(&pool->cond);

    for (int i = 0; i < pool->numCPU; i++) {
        pthread_join(pool->tid[i], NULL);
    }

    free(pool->tid);
    free(pool->task_queue);

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
}

// Main thread (Boss)
int main(int argc, char *argv[]) {
    if (argc < 3) {
        exit(EXIT_FAILURE);
    }
    
    threadpool_t *pool = init_threadpool(argv);

    char c;
    int counter = 0;
    char *data = (char*) malloc(sizeof(char)*1024);

    while ((c = getchar()) != EOF) {
        data[counter] = c;
	  	counter++;

        if (counter == 1024) {
            pthread_mutex_lock(&pool->mutex);
            enqueue(pool->task_queue, data);
            pthread_mutex_unlock(&pool->mutex);
            pthread_cond_signal(&pool->cond);
            counter = 0;
        }
    }

    // work on remaining data
    if (counter > 0) {
        pthread_mutex_lock(&pool->mutex);
        enqueue(pool->task_queue, data);
        pthread_mutex_unlock(&pool->mutex);
        pthread_cond_signal(&pool->cond);
    }

    // wait for workers to finish
    while (pool->task_queue->head != NULL);
    threadpool_shutdown(pool);

    // exit
    exit(EXIT_SUCCESS);
}