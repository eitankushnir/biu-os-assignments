#include <bits/types/sigset_t.h>
#include <signal.h>
#include <stdio.h>

static void print_menu(void);
static void print_round_title(int);
static void initialize_sigprocmask(void);
static void send_signal(int option);
static void proccess_distactions(void);
static void emopty_sigprocmask(void);
static int handle_input(void);

void runFocusMode(int numOfRounds, int duration)
{
    int currentRound = 1;
    printf("Entering Focus Mode. All distractions are blocked.\n");
    while (currentRound <= numOfRounds) {
        initialize_sigprocmask();
        print_round_title(currentRound);
        int distraction_count = 0;
        while (distraction_count < duration) {
            print_menu();
            int input = handle_input();
            if (input == 0)
                break;
            send_signal(input);
            distraction_count++;
        }
        proccess_distactions();
        currentRound++;
    }
    emopty_sigprocmask();
    printf("\nFocus Mode complete. All distractions are now unblocked.\n");
}

static int handle_input()
{
    int input;
    char quit;
    if (scanf("%d", &input) == 1) {
        return input;
    } else if (scanf(" %c", &quit) == 1) {
        if (quit == 'q')
            return 0;
    }
    return -1;
}

static void initialize_sigprocmask()
{
    sigset_t set;
    if (sigemptyset(&set))
        perror("sigemptyset");
    if (sigaddset(&set, SIGUSR1))
        perror("sigaddset USR1");
    if (sigaddset(&set, SIGUSR2))
        perror("sigaddset USR2");
    if (sigaddset(&set, SIGCHLD))
        perror("sigaddset CHLD");
    if (sigprocmask(SIG_SETMASK, &set, NULL))
        perror("sigprocmask");

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGUSR1, &sa, NULL))
        perror("sigaction USR1");
    if (sigaction(SIGUSR2, &sa, NULL))
        perror("sigaction USR2");
    if (sigaction(SIGCHLD, &sa, NULL))
        perror("sigaction CHLD");
}
static void emopty_sigprocmask()
{
    sigset_t set;
    if (sigemptyset(&set))
        perror("sigemptyset");
    if (sigprocmask(SIG_SETMASK, &set, NULL))
        perror("sigprocmask");
}
static void print_round_title(int round_num)
{
    printf("══════════════════════════════════════════════\n");
    printf("                Focus Round %d                \n", round_num);
    printf("──────────────────────────────────────────────\n");
}

static void print_menu()
{
    printf("\n");
    printf("Simulate a distraction:\n");
    printf("  1 = Email notification\n");
    printf("  2 = Reminder to pick up delivery\n");
    printf("  3 = Doorbell Ringing\n");
    printf("  q = Quit\n");
    printf(">> ");
}

static void send_signal(int option)
{
    if (option == 1) {
        if (kill(0, SIGUSR1))
            perror("kill USR1");
    } else if (option == 2) {
        if (kill(0, SIGUSR2)) {
            perror("kill USR1");
        }
    } else if (option == 3) {
        if (kill(0, SIGCHLD))
            perror("kill CHLD");
    } else {
        printf("Invalid Option\n");
    }
}

static void proccess_distactions()
{
    printf("──────────────────────────────────────────────\n");
    printf("        Checking pending distractions...      \n");
    printf("──────────────────────────────────────────────\n");

    sigset_t set;
    int distactions_proccessed = 0;
    if (sigpending(&set))
        perror("sigpending");
    if (sigismember(&set, SIGUSR1)) {
        printf(" - Email notification is waiting.\n");
        printf("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
        distactions_proccessed++;
    }
    if (sigismember(&set, SIGUSR2)) {
        printf(" - You have a reminder to pick up your delivery.\n");
        printf("[Outcome:] You picked it up just in time.\n");
        distactions_proccessed++;
    }
    if (sigismember(&set, SIGCHLD)) {
        printf(" - The doorbell is ringing.\n");
        printf("[Outcome:] Food delivery is here.\n");
        distactions_proccessed++;
    }
    if (distactions_proccessed == 0) {
        printf("No distractions reached you this round.\n");
    }
    printf("──────────────────────────────────────────────\n");
    printf("             Back to Focus Mode.              \n");
    printf("══════════════════════════════════════════════\n");
}
