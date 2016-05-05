#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <zconf.h>

pthread_t philosophers[5];
pthread_mutex_t forks[5];
sem_t waiter;

void getFork(int fid) {
    pthread_mutex_lock(&forks[fid]);
}

void putFork(int fid) {
    pthread_mutex_unlock(&forks[fid]);
}

void *fun(void *phid) {
    int id = *((int *) phid);
    int left = id;
    int right = (id + 1) % 5;
    while (1) {
        usleep(rand() % 100);
        sem_wait(&waiter);
        getFork(left);
        getFork(right);
        printf("Philosopher %d is eating\n", id);
        usleep(rand() % 100);

        putFork(left);
        putFork(right);
        sem_post(&waiter);
    }
    return NULL;
}

int main(int argc, char *argv[]){

    srand(time(NULL));
    sem_init(&waiter, 0, 4);

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        printf("ala ma kota\n");
        exit(1);
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL) != 0) {
        printf("kota ma ale\n");
        exit(2);
    }
    int phids[5];
    for (int i = 0; i < 5; i++) {
        phids[i] = i;
        if (pthread_mutex_init(&forks[i], &attr) != 0) {
            printf("mutex error\n");
            exit(3);
        }
    }

    for (int i = 0; i < 5; i++) {
        if (pthread_create(&philosophers[i], NULL, fun, &phids[i]) != 0) {
            printf("create thread error\n");
            exit(4);
        }
    }
    //while(1);

    for (int i = 0; i < 5; i++) {
        if (pthread_join(philosophers[i], NULL) != 0) {
            printf("error while waiting for thread\n");
            exit(6);
        }
    }
    sem_destroy(&waiter);

    return 0;
}