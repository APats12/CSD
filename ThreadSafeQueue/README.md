# Thread-Safe Queue Implementation

This is a C implementation of a thread-safe queue, designed to be used in a multi-threaded environment where multiple threads may need to add or remove elements from a shared queue.

# Implementation Details

This implementation uses the following queue_t struct:

```c
typedef struct queue {
    void **buffer;  // pointer to the buffer that will hold the elements
    int size;       // maximum size of the buffer
    int count;      // number of elements currently in the buffer
    int head;       // index of the head of the buffer
    int tail;       // index of the tail of the buffer
    pthread_mutex_t lock;   // mutex to ensure thread safety
    pthread_cond_t not_empty;   // condition variable to signal when the buffer is not empty
    pthread_cond_t not_full;    // condition variable to signal when the buffer is not full
} queue_t;
```

The implementation uses a circular buffer with a fixed maximum size to store the elements. The queue supports the following operations:

```c
queue_new(int size): //creates a new queue with the specified maximum size and returns a pointer to the newly created queue.
queue_delete(queue_t **q): //deletes the specified queue and frees all associated resources.
queue_push(queue_t *q, void *elem): //adds the specified element to the end of the queue.
queue_pop(queue_t *q, void **elem): //removes and returns the element at the front of the queue.
```
The implementation uses a mutex to ensure thread safety and two condition variables to signal when the queue is not empty or not full, respectively.

