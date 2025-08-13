#include "bounded_buffer.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bounded_buffer* new_bounded_buffer(int size)
{
    bounded_buffer* buf = malloc(sizeof(bounded_buffer));
    if (buf == NULL) {
        perror("bounded_buffer allocation failed");
        exit(1);
    }
    buf->size = size;
    buf->data = malloc(size * sizeof(char*));
    if (buf->data == NULL) {
        perror("buffer data allocation failed");
        exit(1);
    }
    buf->in = 0;
    buf->out = 0;

    if (sem_init(&buf->mutex, 0, 1) == -1) {
        perror("mutex init failed");
        exit(1);
    }
    if (sem_init(&buf->empty, 0, size) == -1) {
        perror("empty semaphore init failed");
        exit(1);
    }
    if (sem_init(&buf->full, 0, 0) == -1) {
        perror("full semaphore init failed");
        exit(1);
    }

    return buf;
}

void buffer_insert(bounded_buffer* buf, char* s)
{
    sem_wait(&buf->empty);
    sem_wait(&buf->mutex);

    buf->data[buf->in] = s;
    buf->in = (buf->in + 1) % buf->size;

    sem_post(&buf->mutex);
    sem_post(&buf->full);
}

char* buffer_remove(bounded_buffer* buf)
{
    sem_wait(&buf->full);
    sem_wait(&buf->mutex);

    char* s = buf->data[buf->out];
    buf->data[buf->out] = NULL;
    buf->out = (buf->out + 1) % buf->size;

    sem_post(&buf->mutex);
    sem_post(&buf->empty);
    return s;
}

void buffer_free(bounded_buffer* buf)
{
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
    sem_destroy(&buf->mutex);
    for (int i = 0; i < buf->size; i++) {
        free(buf->data[i]);
    }
    free(buf->data);
    free(buf);
}

int buffer_is_empty(bounded_buffer* buf)
{
    return buf->in == buf->out;
}
