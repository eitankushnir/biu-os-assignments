#include "CPU-Scheduler.c"
#include "Focus-Mode.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Usage: %s <Focus-Mode/CPU-Schedule> <Num-Of-Rounds/Processes.csv> <Round-Duration/Time-Quantum>",
            argv[0]);
        exit(0);
    }

    if (!strcmp(argv[1], "Focus-Mode") && !strcmp(argv[1], "CPU-Scheduler")) {
        printf("Usage: %s <Focus-Mode/CPU-Schedule> <Num-Of-Rounds/Processes.csv> <Round-Duration/Time-Quantum>",
            argv[0]);
        exit(0);
    }

    if (strcmp(argv[1], "Focus-Mode") == 0) {
        int numOfRounds = atoi(argv[2]);
        int roundDuration = atoi(argv[3]);
        runFocusMode(numOfRounds, roundDuration);
    }

    if (strcmp(argv[1], "CPU-Scheduler") == 0) {
        char* processesCsvFilePath = argv[2];
        int timeQuantum = atoi(argv[3]);
        runCPUScheduler(processesCsvFilePath, timeQuantum);
    }
}
