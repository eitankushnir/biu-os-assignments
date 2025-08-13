#include "producer.h"

typedef struct editor {
    char* type;
    bounded_buffer* queue;
    int edit_time_ms;
} editor;

typedef struct dispatcher {
    producer** producers;
    editor** editors;

    int prodc;
    int editorc;
} dispatcher;

editor* new_editor(char* type, int edit_time_ms);
dispatcher* new_dispatcher();

void free_editor(editor*);
void free_dispatcher(dispatcher*);
