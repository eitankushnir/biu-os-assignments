#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void create_hardlink(char*, char*);
void replicate_symlink(char*, char*);
void copy_directory(char*, char*);

int main(int argc, char* argv[])
{
    if (argc != 3) {

        exit(1);
    }
    char* src_path = argv[1];
    char* dest_path = argv[2];

    DIR* src = opendir(src_path);
    if (src == NULL) {
        perror("src dir");
        exit(1);
    }
    DIR* dest = opendir(dest_path);
    if (dest != NULL) {
        perror("backup dir");
        exit(1);
    }
    copy_directory(src_path, dest_path);
    return 0;
}

void create_hardlink(char* source_file_path, char* link_path)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/ln", "ln", source_file_path, link_path, NULL);
        perror("exec hardlink failed");
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return;
        } else {
            perror("ln failed to create hard link");
        }
    } else {
        perror("fork hardlink failed");
    }
}

void replicate_symlink(char* link_path, char* backup_link_path)
{
    char target[PATH_MAX];
    ssize_t len = readlink(link_path, target, PATH_MAX);
    if (len < 0) {
        perror("readlink");
        return;
    }
    target[len] = '\0'; // need to null terminate result.
    if (target[0] != '/' && symlink(target, backup_link_path) != 0) {
        perror("symlink");
    }
    // need to replace the absolute path of the symbolic link with the backup.
}

void create_directory(char* path)
{
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/mkdir", "mkdir", path, NULL);
        perror("exec mkdir failed");
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return;
        } else {
            perror("mkdir failed");
        }
    } else {
        perror("fork mkdir failed");
    }
}

void copy_directory(char* src_path, char* dest_path)
{
    DIR* src = opendir(src_path);
    create_directory(dest_path);

    struct dirent* ent;
    while ((ent = readdir(src)) != NULL) {
        // Skip . and ..
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        // Get file stats
        int file_path_len = strlen(src_path) + strlen(ent->d_name) + 1;
        int link_path_len = strlen(dest_path) + strlen(ent->d_name) + 1;
        char* file_path = malloc(file_path_len + 1);
        char* link_path = malloc(link_path_len + 1);
        if (file_path == NULL || link_path == NULL) {
            perror("malloc failed");
            continue;
        }
        snprintf(file_path, file_path_len + 1, "%s/%s", src_path, ent->d_name);
        snprintf(link_path, link_path_len + 1, "%s/%s", dest_path, ent->d_name);

        //  // printf("SOURCE: %s\n", file_path);
        //  // printf("PATHSIZE: %d\n", file_path_len);
        struct stat filestat;
        if (lstat(file_path, &filestat) != 0) {
            perror("stat failed");
            continue;
        }

        // Create hard link for regular files
        if (S_ISREG(filestat.st_mode)) {
            create_hardlink(file_path, link_path);
        } else if (S_ISLNK(filestat.st_mode)) {
            // Replicate existing symbolic links
            replicate_symlink(file_path, link_path);
        } else if (S_ISDIR(filestat.st_mode)) {
            copy_directory(file_path, link_path);
        }
    }
}
