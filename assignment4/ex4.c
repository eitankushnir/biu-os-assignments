#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LINE 256
#define SPORTS 0
#define NEWS 1
#define WEATHER 2

// Global variables
producer *producers;
int *producer_done_flags;
int producer_count = 0;
bounded_buffer *editor_queues[3];
int editor_done_count = 0;
bounded_buffer *sm_queue;

void parse_config_file(char *path)
{
    int prodc = 0;
    FILE *config_file = fopen(path, "r");
    if (config_file == NULL)
    {
        perror("failed to open config file");
        exit(1);
    }
    char line[MAX_LINE];
    producers = malloc(sizeof(producer));

    while (fgets(line, MAX_LINE, config_file))
    {
        if (strstr(line, "PRODUCER"))
        {
            producers[prodc].id = prodc + 1;
            if (fgets(line, MAX_LINE, config_file))
                producers[prodc].product_count = atoi(line);
            if (fgets(line, MAX_LINE, config_file))
                sscanf(line, "queue size = %d", &producers[prodc].queue_size);
            producers[prodc].buffer = new_bounded_buffer(producers[prodc].queue_size);
            prodc++;
            producers = realloc(producers, (prodc + 1) * sizeof(producer));
        }
        if (strstr(line, "Co-Editor queue size"))
        {
            int qsize;
            sscanf(line, "Co-Editor queue size = %d", &qsize);
            editor_queues[SPORTS] = new_bounded_buffer(qsize);
            editor_queues[NEWS] = new_bounded_buffer(qsize);
            editor_queues[WEATHER] = new_bounded_buffer(qsize);
            sm_queue = new_bounded_buffer(qsize);
            printf("QSIZE = %d\n", qsize);
        }
    }
    producer_count = prodc;
    producer_done_flags = calloc(prodc, sizeof(int));
    for (int i = 0; i < prodc; i++)
    {
        printf("PRODICER: %d\n", producers[i].id);
        printf("product_count: %d\n", producers[i].product_count);
        printf("queue size = %d\n", producers[i].queue_size);
    }
}

void *producer_thread(void *arg)
{
    producer *args = (producer *)arg;
    for (int i = 0; i < args->product_count; i++)
    {
        char *content = generate_content(args);
        buffer_insert(args->buffer, content);
    }
    buffer_insert(args->buffer, "DONE");
    return NULL;
}

void *dispatcher_thread(void *arg)
{
    int done_count = 0;
    while (done_count < producer_count)
    {
        for (int i = 0; i < producer_count; i++)
        {
            if (producer_done_flags[i])
                continue;
            char *msg = buffer_remove(producers[i].buffer);
            if (strcmp(msg, "DONE") == 0)
            {
                producer_done_flags[i] = 1;
                done_count++;
                continue;
            }
            char type[10];
            int tmp;
            sscanf(msg, "Producer %d %s", &tmp, type);
            if (strcmp(type, "SPORTS") == 0)
                buffer_insert(editor_queues[SPORTS], msg);
            if (strcmp(type, "NEWS") == 0)
                buffer_insert(editor_queues[NEWS], msg);
            if (strcmp(type, "WEATHER") == 0)
                buffer_insert(editor_queues[WEATHER], msg);
        }
    }
    buffer_insert(editor_queues[SPORTS], "DONE");
    buffer_insert(editor_queues[NEWS], "DONE");
    buffer_insert(editor_queues[WEATHER], "DONE");
}

void *editor_thread(void *arg)
{
    bounded_buffer *queue = (bounded_buffer *)arg;
    char *msg = buffer_remove(queue);
    while (strcmp(msg, "DONE") != 0)
    {
        // sleep
        buffer_insert(sm_queue, msg);
        buffer_remove(queue);
    }
    editor_done_count++;
    return NULL;
}

void *screen_manager_thread(void *arg)
{
    while (editor_done_count < 3 || buffer_is_empty(sm_queue))
    {
        char *msg = buffer_remove(sm_queue);
        printf("%s\n", msg);
    }
    printf("DONE");
    return NULL;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    parse_config_file(argv[1]);
    pthread_t p;
    for (int i = 0; i < producer_count; i++)
    {
        pthread_create(&p, NULL, producer_thread, &producers[i]);
    }
    pthread_create(&p, NULL, dispatcher_thread, NULL);
    pthread_create(&p, NULL, editor_thread, editor_queues[SPORTS]);
    pthread_create(&p, NULL, editor_thread, editor_queues[NEWS]);
    pthread_create(&p, NULL, editor_thread, editor_queues[WEATHER]);
    pthread_create(&p, NULL, screen_manager_thread, NULL);
    pthread_join(p, NULL);
    return 0;
}
