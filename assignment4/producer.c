#include "producer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* generate_content(producer* producer)
{
    const char* types[] = { "SPORTS", "NEWS", "WEATHER" };
    int idx = rand() % 3;
    const char* type = types[idx];
    int c = producer->content_generated[idx];
    int len = snprintf(0, 0, "Producer %d %s %d", producer->id, type, c);
    char* content = malloc(len + 1);
    if (content == NULL) {
        perror("generate_content malloc failed");
        exit(1);
    }
    snprintf(content, len, "Producer %d %s %d", producer->id, type, c);
    return content;
}

producer* new_producer(int id, int product_count, int queue_size)
{
    producer* p = malloc(sizeof(producer));
    if (p == NULL) {
        perror("producer allocation failed");
        exit(1);
    }
    p->id = id;
    p->queue_size = queue_size;
    p->product_count = product_count;
    memset(p->content_generated, 0, sizeof(p->content_generated));
    p->buffer = new_bounded_buffer(queue_size);
    return p;
}

void free_producer(producer* p)
{
    buffer_free(p->buffer);
    free(p);
}
