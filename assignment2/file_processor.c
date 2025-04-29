#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s <data.txt> <requests.txt\n", argv[0]);
        exit(1);
    }
    FILE* data = fopen(argv[1], "r+");
    if (data == NULL && errno == ENOENT) {
        perror("data.txt");
        exit(1);
    }
    FILE* requests = fopen(argv[2], "r");
    if (requests == NULL && errno == ENOENT) {
        perror("requests.txt");
        exit(1);
    }

    char cmd;
    while (fscanf(requests, " %c", &cmd) == 1) {
        if (cmd == 'Q')
            break;
        else if (cmd == 'R') {
            int start, end;
            fscanf(requests, "%d %d", &start, &end);

        } else if (cmd == 'W') {
            int offset;
            char text[256];
            fscanf(requests, "%d %s", &offset, text);
        }
    }

    return 0;
}

void readline(FILE* requests, char* cmdptr, int args[2])
{
}
