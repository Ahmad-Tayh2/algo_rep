#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NUM_CHILDREN 4
#define SEM_NAME "/sem_start"

sem_t *sem;

void handle_signal(int sig)
{
    if (sig == SIGUSR1)
    {
        printf("Processus %d : Signal de démarrage reçu, exécution de la tâche...\n", getpid());
        sleep(2);
        printf("Processus %d : Tâche terminée.\n", getpid());

        kill(getppid(), SIGUSR2);
    }
}

void parent_signal_handler(int sig)
{
    static int received_signals = 0;

    if (sig == SIGUSR2)
    {
        received_signals++;
        printf("Processus père : Confirmation reçue d'un fils (%d/%d).\n", received_signals, NUM_CHILDREN);

        if (received_signals == NUM_CHILDREN)
        {
            printf("Processus père : Tous les fils ont terminé leurs tâches.\n");
            sem_close(sem);
            sem_unlink(SEM_NAME);
            exit(0);
        }
    }
}

int main()
{
    pid_t pids[NUM_CHILDREN];

    // Ouvrir le sémaphore
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR2, parent_signal_handler);

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            signal(SIGUSR1, handle_signal);

            sem_wait(sem);

            while (1)
            {
                pause();
            }
            exit(0);
        }
    }

    sleep(1);

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        sem_post(sem);
    }

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        kill(pids[i], SIGUSR1);
    }

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        wait(NULL);
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}
