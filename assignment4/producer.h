#include "bounded_buffer.h"

#define SPORTS 0
#define NEWS 1
#define WEATHER 2

typedef struct producer {
    int id;
    int product_count;
    int queue_size;
    bounded_buffer* buffer;

    int content_generated[3];
} producer;

char* generate_content(producer*);
producer* new_producer(int id, int product_count, int queue_size);
void free_producer(producer*);
