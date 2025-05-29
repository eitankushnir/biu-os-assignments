#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_NAME 51
#define MAX_DESCRIPTION 101
#define MAX_LINE 256
#define MAX_PROCESSES 1000

typedef struct {
    char name[MAX_NAME];
    char description[MAX_DESCRIPTION];
    int arrival_time;
    unsigned int burst_time;
    unsigned int priority;

    int remaining_time;
    int start_time;
    int end_time;

    int index;
    int finished;
} process;

typedef int (*comparator)(const process*, const process*);
typedef void (*scheduler)(process* processes, int process_count, int time_quantum);

int read_processes_file(char* path, process* buf);
void mergesort(process*, int r, int l, comparator);
void run_fcfs_scheduler(process*, int, int);
void run_sjf_scheduler(process*, int, int);
void run_priority_scheduler(process*, int, int);
void run_rr_scheduler(process*, int p_count, int quantum);
void run_scheduler(scheduler, int);

int arival_time_cmp(const process* p1, const process* p2)
{
    return p1->arrival_time - p2->arrival_time;
}
int remaining_time_cmp(const process* p1, const process* p2)
{
    return p1->remaining_time - p2->remaining_time;
}
int priority_cmp(const process* p1, const process* p2)
{
    return p1->priority - p2->priority;
}
void alarm_hand(int signum)
{
    signal(SIGALRM, alarm_hand);
}

process processes[MAX_PROCESSES];
int current_time = 0;
int process_count = 0;

void runCPUScheduler(char* processesCsvFilePath, int timeQuantum)
{
    signal(SIGALRM, alarm_hand);
    process_count = read_processes_file(processesCsvFilePath, processes);
    if (process_count < 0)
        exit(1);

    run_scheduler(run_fcfs_scheduler, 0);
    printf("\n");
    run_scheduler(run_sjf_scheduler, 0);
    printf("\n");
    run_scheduler(run_priority_scheduler, 0);
    printf("\n");
    run_scheduler(run_rr_scheduler, timeQuantum);
}

void run_scheduler(scheduler scheduler, int time_quantum)
{
    fflush(stdout);
    pid_t scheduler_pid = fork();
    if (scheduler_pid == 0) {
        // Scheduler
        scheduler(processes, process_count, time_quantum);
        exit(0);
    } else if (scheduler_pid > 0) {
        // parent
        wait(NULL);
    } else {
        perror("fork failed");
        exit(1);
    }
}
void run_process(process* p, int duration)
{
    alarm(duration);
    pause();

    printf("%d → %d: %s Running %s.\n", current_time, current_time + duration, p->name, p->description);
    fflush(stdout);
    if (p->start_time == -1) {
        p->start_time = current_time;
    }
    current_time += duration;
    p->remaining_time -= duration;
    if (p->remaining_time <= 0) {
        p->finished = 1;
        p->end_time = current_time;
    } else {
        p->arrival_time = current_time;
    }
}
void idle(int duration)
{
    alarm(duration);
    pause();

    printf("%d → %d: Idle.\n", current_time, current_time + duration);
    fflush(stdout);
    current_time += duration;
}
double average_wait_time(process* processes, int process_count)
{
    int total_wait_time = 0;
    for (int i = 0; i < process_count; i++) {
        process* p = &processes[i];
        int wait_time = p->start_time - p->arrival_time;
        total_wait_time += wait_time;
    }
    return (double)total_wait_time / process_count;
}

// Reads all process from the given file into buffer.
// Returns the number of processes read or -1 on failure.
int read_processes_file(char* processesCsvFilePath, process* buffer)
{
    FILE* processfile = fopen(processesCsvFilePath, "r");
    if (processfile == NULL) {
        perror("fopen failed");
        return -1;
    }
    int i = 0;
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, processfile)) {
        process* p = &processes[i];
        sscanf(line, "%50[^,],%100[^,],%d,%u,%u",
            p->name,
            p->description,
            &p->arrival_time,
            &p->burst_time,
            &p->priority);
        p->remaining_time = p->burst_time;
        p->start_time = -1;
        p->end_time = -1;
        p->index = i;
        p->finished = 0;
        i++;
    }
    if (fclose(processfile)) {
        perror("fclose failed");
        return -1;
    }
    return i;
}

void run_fcfs_scheduler(process* processes, int process_count, int unused)
{
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : FCFS\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    mergesort(processes, 0, process_count - 1, arival_time_cmp);

    int processes_finished = 0;
    while (processes_finished < process_count) {
        process* p = &processes[processes_finished];
        if (p->arrival_time <= current_time) {
            run_process(p, p->burst_time);
            processes_finished++;
        } else {
            idle(p->arrival_time - current_time);
        }
    }
    printf("\n");
    printf("──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2lf time units\n", average_wait_time(processes, process_count));
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n");
}

void run_sjf_scheduler(process* processes, int process_count, int unused)
{
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : SJF\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    mergesort(processes, 0, process_count - 1, arival_time_cmp);
    mergesort(processes, 0, process_count - 1, remaining_time_cmp);

    int processes_finished = 0;
    while (processes_finished < process_count) {
        int next_arival = -1;
        int process_in_queue = 0;
        for (int i = 0; i < process_count; i++) {
            process* p = &processes[i];
            if (p->finished)
                continue;
            if (p->arrival_time <= current_time) {
                run_process(p, p->remaining_time);
                processes_finished++;
                process_in_queue = 1;
                break;
            } else if (next_arival == -1)
                next_arival = p->arrival_time;
            else
                next_arival = (next_arival > p->arrival_time) ? p->arrival_time : next_arival;
        }
        if (!process_in_queue)
            idle(next_arival - current_time);
    }
    printf("\n");
    printf("──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2lf time units\n", average_wait_time(processes, process_count));
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n");
}

void run_priority_scheduler(process* processes, int process_count, int unused)
{
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Priority\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    mergesort(processes, 0, process_count - 1, arival_time_cmp);
    mergesort(processes, 0, process_count - 1, priority_cmp);

    int processes_finished = 0;
    while (processes_finished < process_count) {
        int next_arival = -1;
        int process_in_queue = 0;
        for (int i = 0; i < process_count; i++) {
            process* p = &processes[i];
            if (p->finished)
                continue;
            if (p->arrival_time <= current_time) {
                run_process(p, p->remaining_time);
                processes_finished++;
                process_in_queue = 1;
                break;
            } else if (next_arival == -1)
                next_arival = p->arrival_time;
            else
                next_arival = (next_arival > p->arrival_time) ? p->arrival_time : next_arival;
        }
        if (!process_in_queue)
            idle(next_arival - current_time);
    }
    printf("\n");
    printf("──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2lf time units\n", average_wait_time(processes, process_count));
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n");
}

void run_rr_scheduler(process* processes, int process_count, int time_quantum)
{
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : Round Robin\n");
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");

    mergesort(processes, 0, process_count - 1, arival_time_cmp);

    int processes_finished = 0;
    while (processes_finished < process_count) {
        int next_arrival = processes[process_count - 1].arrival_time;
        int process_in_queue = 0;
        for (int i = 0; i < process_count; i++) {
            process* p = &processes[i];
            if (p->finished)
                continue;
            if (p->arrival_time <= current_time) {
                int runtime = (time_quantum <= p->remaining_time) ? time_quantum : p->remaining_time;
                run_process(p, runtime);
                processes_finished += p->finished;
                process_in_queue = 1;
                mergesort(processes, 0, process_count - 1, arival_time_cmp);
                break;
            } else {
                next_arrival = (next_arrival > p->arrival_time) ? p->arrival_time : next_arrival;
            }
        }
        if (!process_in_queue)
            idle(next_arrival - current_time);
    }
    printf("\n");
    printf("──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Total Turnaround Time : %d time units\n\n", current_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n");
}

void merge(process* processes, int l, int m, int r, comparator comparator)
{
    int lm = m - l + 1;
    int mr = r - m;
    process left[MAX_PROCESSES], right[MAX_PROCESSES];
    for (int i = 0; i < lm; i++) {
        left[i] = processes[l + i];
    }
    for (int i = 0; i < mr; i++) {
        right[i] = processes[m + 1 + i];
    }
    int i = 0, j = 0, k = l;

    while (i < lm && j < mr) {
        if (comparator(&left[i], &right[j]) <= 0) {
            processes[k] = left[i];
            i++;
        } else {
            processes[k] = right[j];
            j++;
        }
        k++;
    }

    while (i < lm) {
        processes[k] = left[i];
        i++;
        k++;
    }
    while (j < mr) {
        processes[k] = right[j];
        j++;
        k++;
    }
}

void mergesort(process* processes, int l, int r, comparator comparator)
{
    if (l >= r) {
        return;
    }
    int m = (r + l) / 2;
    mergesort(processes, l, m, comparator);
    mergesort(processes, m + 1, r, comparator);
    merge(processes, l, m, r, comparator);
}
