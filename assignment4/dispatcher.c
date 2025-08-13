#include "editor.h"
#include <stdlib.h>
#include <string.h>

editor* new_editor(char* type, int edit_time_ms)
{
    editor* e = malloc(sizeof(editor));
    e->type = malloc(strlen(type) + 1);
    strncpy(e->type, type, strlen(type));
    e->edit_time_ms = edit_time_ms;
    return e;
}

dispatcher* new_dispatcher()
{
    dispatcher* d = calloc(1, sizeof(dispatcher));
    return d;
}

void dispatcher_add_producer(dispatcher* dispatcher, producer* producer)
{
    if (dispatcher->prodc == 0) {
        dispatcher->producers = malloc(sizeof(producer*));
        dispatcher->producers[0] = producer;
        return;
    }
    dispatcher->producers = realloc(dispatcher->producers, dispatcher->prodc + 1);
    dispatcher->producers[dispatcher->prodc++] = producer;
}
