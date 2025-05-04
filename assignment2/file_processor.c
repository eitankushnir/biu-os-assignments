#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void handle_read(int data_fd, int results_fd, off_t start, off_t end, off_t file_size);
void handle_write(int data_fd, const char* filename, off_t offset, char* text, off_t file_size);

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s /path/to/data.txt /path/to/requests.txt\n", argv[0]);
        exit(1);
    }

    const char* data_path = argv[1];
    const char* requests_path = argv[2];

    int data_fd = open(data_path, O_RDWR);
    if (data_fd == -1) {
        perror("data.txt");
        exit(1);
    }
    FILE* requests_file = fopen(requests_path, "r");
    if (requests_file == NULL) {
        perror("requests.txt");
        close(data_fd);
        exit(1);
    }

    int results_fd = open("read_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (results_fd == -1) {
        perror("read_results.txt");
        close(data_fd);
        fclose(requests_file);
        exit(1);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), requests_file)) {
        if (line[0] == 'Q')
            break;
        struct stat st;
        if (fstat(data_fd, &st) == -1) {
            perror("fstat");
            continue;
        }
        off_t file_size = st.st_size;

        if (line[0] == 'R') {
            off_t start, end;
            if (sscanf(line + 1, "%ld %ld", &start, &end) == 2)
                handle_read(data_fd, results_fd, start, end, file_size);
        } else if (line[0] == 'W') {
            off_t offset;
            char text[BUFFER_SIZE];
            if (sscanf(line + 1, "%ld %[^\n]", &offset, text) == 2) {
                handle_write(data_fd, data_path, offset, text, file_size);
                close(data_fd);
                data_fd = open(data_path, O_RDWR);
                if (data_fd == -1) {
                    perror("reopen data.txt");
                    break;
                }
            }
        }
    }
}

// Reads the data inside the data file between offset and end(included) and appends the result to results file
void handle_read(int data_fd, int results_fd, off_t start, off_t end, off_t file_size)
{
    if (start > end || start >= file_size || end >= file_size)
        return;

    off_t len = end - start + 1;
    char* buffer = (char*)malloc(sizeof(char) * (len + 1));
    if (buffer == NULL) {
        perror("malloc read");
        return;
    }

    if (lseek(data_fd, start, SEEK_SET) == -1) {
        perror("lseek read");
        return;
    }

    ssize_t bytes_read = read(data_fd, buffer, len);
    if (bytes_read == -1) {
        perror("read");
        return;
    }

    write(results_fd, buffer, bytes_read);
    write(results_fd, "\n", 1);
}

// Inserts the text into the data file starting from the given offset
void handle_write(int data_fd, const char* filename, off_t offset, char* text, off_t file_size)
{
    if (offset > file_size)
        return;

    size_t text_len = strlen(text);
    if (lseek(data_fd, offset, SEEK_SET) == -1) {
        perror("lseek write");
        return;
    }

    char* tail = malloc(sizeof(char) * (file_size - offset + 1));
    if (tail == NULL) {
        perror("malloc write");
        return;
    }

    ssize_t tail_len = read(data_fd, tail, file_size - offset);
    if (tail_len == -1) {
        perror("read tail");
        free(tail);
        return;
    }

    size_t new_len = offset + text_len + tail_len;
    char* new_data = malloc(new_len);
    if (new_data == NULL) {
        perror("malloc new_data");
        free(tail);
        return;
    }
    if (lseek(data_fd, 0, SEEK_SET) == -1) {
        perror("lseek after new_data");
        free(tail);
        free(new_data);
        return;
    }
    if (read(data_fd, new_data, offset) != offset) {
        perror("read new_data");
        free(tail);
        free(new_data);
        return;
    }
    memcpy(new_data + offset, text, text_len);
    memcpy(new_data + offset + text_len, tail, tail_len);

    int output_fd = open(filename, O_TRUNC | O_WRONLY, 0644);
    if (output_fd == -1) {
        perror("output open");
        free(tail);
        free(new_data);
        return;
    }
    if (write(output_fd, new_data, new_len) != new_len) {
        perror("new data write");
    }

    close(output_fd);
    free(tail);
    free(new_data);
}
