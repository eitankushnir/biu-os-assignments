#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#define NUM_GLADIATORS 4

void run_gladiator(char*);
void compile_gladiator_program();

int main(int argc, char* argv[])
{
    compile_gladiator_program();
    int numberOfDeaths = 0;
    char* gladiator_names[NUM_GLADIATORS] = { "Maximus", "Lucius", "Commodus", "Spartacus" };
    char* gladiator_files[NUM_GLADIATORS] = { "G1", "G2", "G3", "G4" };

    pid_t pids[NUM_GLADIATORS];
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            return 1;
        }

        if (pids[i] == 0) {
            run_gladiator(gladiator_files[i]);
            return 0;
        }
    }

    pid_t last_pid = -1;
    while (numberOfDeaths < NUM_GLADIATORS) {

        int status;
        pid_t pid = wait(&status);
        if (pid > 0) {
            numberOfDeaths++;
            last_pid = pid;
        } else {
            perror("wait failed");
            break;
        }
    }
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        if (last_pid == pids[i]) {
            printf("The gods have spoken, the winner of the tournament is %s!\n", gladiator_names[i]);
            break;
        }
    }
}

void run_gladiator(char* stats_file_path)
{
    if (execl("./gladiator", "gladiator", stats_file_path, NULL) == -1) {
        perror("exec failed");
    }
}

void compile_gladiator_program()
{
    pid_t pid = fork();
    if (pid == 0) {
        if (execl("/usr/bin/gcc", "gcc", "-o", "gladiator", "gladiator.c", NULL) == -1)
            perror("exec failed");
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
    }
}
