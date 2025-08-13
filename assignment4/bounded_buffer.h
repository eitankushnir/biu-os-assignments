#include <pthread.h>
#include <semaphore.h>
typedef struct bounded_buffer {
    int size;
    char** data;
    int in;
    int out;
    sem_t mutex;
    sem_t empty;
    sem_t full;
} bounded_buffer;

bounded_buffer* new_bounded_buffer(int size);
void buffer_insert(bounded_buffer* buf, char* s);
char* buffer_remove(bounded_buffer* buf);
void buffer_free(bounded_buffer* buf);
int buffer_is_empty(bounded_buffer* buf);
