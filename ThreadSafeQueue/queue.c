#include "queue.h"
#include <stdlib.h>
#include <pthread.h>

typedef struct queue {
    void **buffer; // pointer to the buffer that will hold the elements
    int size; // maximum size of the buffer
    int count; // number of elements currently in the buffer
    int head; // index of the head of the buffer
    int tail; // index of the tail of the buffer
    pthread_mutex_t lock; // mutex to ensure thread safety
    pthread_cond_t not_empty; // condition variable to signal when the buffer is not empty
    pthread_cond_t not_full; // condition variable to signal when the buffer is not full
} queue_t;

// function to initialize the queue
queue_t *queue_new(int size) {
    queue_t *q = malloc(sizeof(queue_t));
    q->buffer = malloc(sizeof(void *) * size);
    q->size = size;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

// function to delete the queue
void queue_delete(queue_t **q) {
    pthread_mutex_lock(&(*q)->lock);
    free((*q)->buffer);
    (*q)->buffer = NULL;
    pthread_mutex_unlock(&(*q)->lock);
    pthread_mutex_destroy(&(*q)->lock);
    pthread_cond_destroy(&(*q)->not_empty);
    pthread_cond_destroy(&(*q)->not_full);
    free(*q);
    *q = NULL;
}

// function to add an element to the queue
bool queue_push(queue_t *q, void *elem) {
    pthread_mutex_lock(&q->lock);
    while (q->count == q->size) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->buffer[q->tail] = elem;
    q->tail = (q->tail + 1) % q->size;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return true;
}

// function to remove an element from the queue
bool queue_pop(queue_t *q, void **elem) {
    pthread_mutex_lock(&q->lock);
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    *elem = q->buffer[q->head];
    q->head = (q->head + 1) % q->size;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return true;
}
