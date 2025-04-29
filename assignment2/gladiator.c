#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void get_my_stats(char*, int*, int*, int a[3]);
int get_opponent_attack(int n);

int main(int argc, char* argv[])
{
    if (argc > 2) {
        printf("Usage: %s <gladiator_stats_txt_file_no_extension>\n", argv[0]);
        exit(1);
    }
    char* logpath = (char*)malloc(sizeof(char) * (strlen(argv[1]) + 9));
    strcpy(logpath, argv[1]);
    strcat(logpath, "_log.txt");
    FILE* logFile = fopen(logpath, "w");
    if (logFile == NULL) {
        perror("fopen failed");
        exit(1);
    }
    int health, attack, opponents[3];
    char* statpath = (char*)malloc(sizeof(char) * (strlen(argv[1]) + 5));
    strcpy(statpath, argv[1]);
    strcat(statpath, ".txt");
    get_my_stats(statpath, &health, &attack, opponents);
    // printf("Hp: %d\n", health);
    // printf("Atk: %d\n", attack);
    // printf("Op 1: %d\n", opponents[0]);
    // printf("Op 2: %d\n", opponents[1]);
    // printf("Op 3: %d\n", opponents[2]);

    fprintf(logFile, "Gladiator process started. %d:\n", getpid());
    while (health > 0) {
        for (int i = 0; i < 3; i++) {
            int opponent_attack = get_opponent_attack(opponents[i]);
            fprintf(logFile, "Facing opponent %d... Taking %d damage\n", opponents[i], opponent_attack);
            health -= opponent_attack;
            if (health > 0) {
                fprintf(logFile, "Are you not entertained? Remaining health: %d\n", health);
            } else {
                fprintf(logFile, "The gladiator has fallen... Final health: %d\n", health);
                break;
            }
        }
    }
    fclose(logFile);
    return 0;
}

void get_my_stats(char* statpath, int* health, int* attack, int opponents[3])
{
    FILE* gfile = fopen(statpath, "rb");
    if (gfile == NULL) {
        perror("fopen failed");
    }
    fscanf(gfile, "%d, %d, %d, %d, %d", health, attack, &opponents[0], &opponents[1], &opponents[2]);
    fclose(gfile);
}

int get_opponent_attack(int n)
{
    char opstats[7];
    snprintf(opstats, sizeof(opstats), "G%d.txt", n);
    FILE* opFile = fopen(opstats, "rb");
    if (opFile == NULL) {
        perror("fopen failed");
    }
    int hp, attack;
    fscanf(opFile, "%d, %d", &hp, &attack);
    fclose(opFile);
    return attack;
}
