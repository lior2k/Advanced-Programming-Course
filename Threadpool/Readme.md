# Threadpool Implementation

This project is about implementing a threadpool mechanism to parallelize an encryption algorithm that is not so fast. The encryption algorithm is provided as a shared library compiled for x86, with two functions: `encode` and `decode`. The goal is to utilize multithreading to improve performance by utilizing multi-cores of the system.

## Implementation

The `threadpool` module consists of the following components:

### Queue

- `queue_t` - a structure that represents the task queue, which is used to store the incoming tasks to be executed by the worker threads. It is implemented as a singly-linked list.

### Task

- `task_t` - a structure that represents a task. It contains a pointer to the data to be processed, as well as an ID.

### Threadpool

- `threadpool_t` - a structure that represents the threadpool. It contains the following fields:

  - `tid` - an array of thread IDs.
  - `numCPU` - the number of available CPUs on the system (logical processors) multiplied by 4.
  - `task_queue` - a pointer to the task queue.
  - `mutex` - a mutex used to protect the task queue.
  - `cond` - a condition variable used to signal the worker threads when there is work to do.
  - `sync_order_mutex` - a mutex used to synchronize the order of the output.
  - `sync_order_cond` - a condition variable used to signal that the id of the current output has changed and the next task can be printed.
  - `func` - a pointer to the encryption or decryption function to be used.
  - `key` - the encryption or decryption key to be used.
  - `current_task` - an ID of the next task to be printed out.

### Functions

- `enqueue` - adds a new task to the task queue.
- `dequeue` - removes the first task from the task queue.
- `worker_thread` - the function that is executed by the worker threads. It waits for a signal from the main thread, then dequeues a task from the task queue, processes it, and signals the main thread when it is done.
- `init_threadpool` - initializes the threadpool with the encryption or decryption key and function, and creates the worker threads.

## Example

The following commands can be used to compile and run the example:

```sh
gcc threadpool.c -pthread ./libCodec.so -o exe
echo "Hello, world!" | ./exe 2 -e
outputs "Fcjjm*umpj"
echo "Fcjjm*umpj" | ./exe 2 -d
outputs "Hello, world!"
```

Note that the `-e` flag is used for encryption and the `-d` flag is used for decryption. The number `2` is the encryption or decryption key. The input is taken from `stdin` and the output is printed to `stdout`.