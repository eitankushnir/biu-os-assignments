// External Libraries.
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// MACROS
#define MAX_PATH_LENGTH 1024
#define MAX_FILENAME_LENGTH 256
#define MAX_FILE_COUNT_IN_DIRECTORY 100

// Function Declerations.
int validate_args(int argc, char* argv[]);
int init(char*);
int file_exists(const char* path);
int directory_exists(const char* path);
int create_directory(const char* path);
int cmp_mod_dates(const char*, const char*);
int copy_file(const char*, const char*);
int files_differ(const char*, const char*);
int directory_exists(const char* path);
int sync_directories(const char*, const char*);
int get_files_in_directory(const char* path, char buf[MAX_FILE_COUNT_IN_DIRECTORY][MAX_FILENAME_LENGTH], int*);
void printcwd();

int main(int argc, char* argv[])
{
    printcwd();
    if (validate_args(argc, argv) != 0)
        exit(1);
    if (init(argv[2]) != 0)
        exit(1);

    sync_directories(argv[1], argv[2]);
    return 0;
}

// Returns 0 if the argument count is correct and argv[1] is an existing directory.
// else returns 1.
int validate_args(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: file_sync <source_directory> <destination_directory>\n");
        return 1;
    }
    if (!directory_exists(argv[1])) {
        printf("Error: Source directory '%s' does not exist.\n", argv[1]);
        return 1;
    }
    return 0;
}

// Prints the cwd, using the getcwd() function.
void printcwd()
{
    char cwd[MAX_PATH_LENGTH];
    // pwd
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() failed");
    }
}

// Creates the destination directory at the given path, if it does not already exist.
int init(char* dest)
{
    if (!directory_exists(dest)) {
        printf("Created destination directory '%s'.\n", dest);
        create_directory(dest);
    }
    return 0;
}

// Creates the directory at the given path, using execvp(), fork() and mkdir.
// Returns 0 if successful.
int create_directory(const char* path)
{
    char* args[] = { "mkdir", path, NULL };
    pid_t pid = fork();
    if (pid == 0) { // Child process creates the directory.
        execvp("/bin/mkdir", args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process waits on the child to complete.
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WIFEXITED(status);
        } else {
            perror("mkdir command failed");
            return -1;
        }
    } else {
        perror("fork failed");
        return -1;
    }
}

// Checks whether the file at the specified path exists using stat().
// Returns 1 if it does, else 0.
int file_exists(const char* path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}
// Checks whether the directory at the specified path exists using opendir().
// Returns 1 if it does, 0 if it doesnt, and -1 if opendir() failed.
int directory_exists(const char* path)
{
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;
    } else if (ENOENT == errno) {
        return 0;
    } else {
        perror("opendir() failed");
        return -1;
    }
}

// Compare 2 files modification time.
// returns -1 if f1 is older than f2, 1 if f1 is newer than f2, 0 if they were created at the same time.
int cmp_mod_dates(const char* f1, const char* f2)
{
    struct stat f1buf;
    struct stat f2buf;
    if (stat(f1, &f1buf) != 0 || stat(f2, &f2buf) != 0) {
        printf("Error: Invalid paths input to cmp_mod_dates.\n");
        return 2;
    }
    struct timespec t1, t2;
    t1 = f1buf.st_mtim;
    t2 = f2buf.st_mtim;
    if (t1.tv_sec == t2.tv_sec)
        return (t1.tv_nsec > t2.tv_nsec) - (t2.tv_nsec > t1.tv_nsec);
    else
        return (t1.tv_sec > t2.tv_sec) ? 1 : -1;
}

// Copies a file at src to dest using fork(), execvp() and the cp shell command.
// Return 0 if successful.
int copy_file(const char* src, const char* dest)
{
    char* args[] = { "cp", src, dest, NULL };
    pid_t pid = fork();
    if (pid == 0) { // Child process executes the cp command.
        execvp("/bin/cp", args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process waits on the child process to finish.
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WIFEXITED(status);
        } else {
            perror("cp command failed");
            return -1;
        }
    } else {
        perror("fork failed");
        return -1;
    }
}

// runs the diff shell command on files at the 2 specified paths. using fork(), execvp() and the diff shell command.
// The exit status of diff is returned, or -1 if there was any errors getting to run it.
int files_differ(const char* f1, const char* f2)
{
    char* args[] = { "diff", "-q", f1, f2, NULL };
    pid_t pid = fork();
    if (pid == 0) { // Child process runs the diff command.
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, STDOUT_FILENO);
        close(null_fd);
        execvp("/bin/diff", args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process waits for the child process to finish.
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            perror("diff command failed");
            return -1;
        }
    } else {
        perror("fork failed");
        return -1;
    }
}

// Synchronize the destination with the source.
// Copy any files from source to destination if the source contains a newer version
// or if the destination doesnt contain the source file at all.
// Returns 0 if successful.
int sync_directories(const char* src, const char* dest)
{
    // Get the list and count of all file names in the source directory.
    char dir_contents[MAX_FILE_COUNT_IN_DIRECTORY][MAX_FILENAME_LENGTH] = { 0 };
    int dir_contents_size;
    get_files_in_directory(src, dir_contents, &dir_contents_size);

    // Get the absolute paths of both source and dest.
    char src_abs_path[MAX_PATH_LENGTH];
    char dest_abs_path[MAX_PATH_LENGTH];

    if (realpath(src, src_abs_path) == NULL) {
        perror("src realpath() failed");
        return 0;
    }
    if (realpath(dest, dest_abs_path) == NULL) {
        perror("dest realpath() failed");
        return 0;
    }
    printf("Synchronizing from %s to %s\n", src_abs_path, dest_abs_path);
    // Run over all files in the source directory and synchronize with the destination.
    for (int i = 0; i < dir_contents_size; i++) {
        // Get absolute paths of the current file in the source and destination.
        char src_filename[MAX_PATH_LENGTH + MAX_FILENAME_LENGTH], dest_filename[MAX_PATH_LENGTH + MAX_FILENAME_LENGTH];
        char* base_filename = dir_contents[i];
        snprintf(src_filename, sizeof(src_filename), "%s/%s", src_abs_path, base_filename);
        snprintf(dest_filename, sizeof(dest_filename), "%s/%s", dest_abs_path, base_filename);

        struct stat filestat;
        if (stat(src_filename, &filestat) == 0 && S_ISREG(filestat.st_mode)) {
            if (!file_exists(dest_filename)) { // If file doesnt exist in destination, copy it over.
                printf("New file found: %s\n", base_filename);
                copy_file(src_filename, dest_filename);
                printf("Copied: %s -> %s\n", src_filename, dest_filename);
            } else if (files_differ(src_filename, dest_filename)) { // If it does, check if the files have any differences.
                if (cmp_mod_dates(src_filename, dest_filename) > 0) { // Update file in destination if it is older, otherwise skip it.
                    printf("File %s is newer in source. Updating...\n", base_filename);
                    copy_file(src_filename, dest_filename);
                    printf("Copied: %s -> %s\n", src_filename, dest_filename);
                } else {
                    printf("File %s is newer in destination. Skipping...\n", base_filename);
                }
            } else { // Skip copying if both files have the same content.
                printf("File %s is identical. Skipping...\n", base_filename);
            }
        }
    }
    printf("Synchronization complete.\n");
    return 0;
}

int string_comparator(const void* s1, const void* s2)
{
    return strcmp((const char*)s1, (const char*)s2);
}

// Fills the buffer array with all file names inside the directory at the given path, sorted alphabetically.
// Sets the value at sizep to be the amount of files in the directory.
// files mean only regular files.
// Returns 0 if successful.
int get_files_in_directory(const char* path, char buf[MAX_FILE_COUNT_IN_DIRECTORY][MAX_FILENAME_LENGTH], int* sizep)
{
    DIR* dir = opendir(path);
    if (dir == NULL) {
        perror("Unable to open source directory");
        return 1;
    }
    int i = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        // Get the path of the file in the array.
        char file_path[MAX_PATH_LENGTH];
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        // Check if file exists and is a regular file.
        struct stat filestat;
        if (stat(file_path, &filestat) == 0 && S_ISREG(filestat.st_mode)) {
            strcpy(buf[i++], entry->d_name); // add file name to the buffer.
        }
    }
    closedir(dir);
    qsort(buf, i, MAX_FILENAME_LENGTH, string_comparator); // sort the buffer alphabetically.
    *sizep = i;
    return 0;
}
