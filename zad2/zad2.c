#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <zconf.h>
#include <signal.h>

typedef struct plane {
    int id;
    volatile int alive;
} plane;


pthread_mutex_t mtx;
pthread_cond_t starting;
pthread_cond_t landing;
int planes_num;
int n;
int k;
volatile int wait_to_start = 0;
volatile int wait_to_land = 0;
volatile int isfree = 1;
volatile int on_board = 0;
pthread_t *thread_ids;
struct plane *planes;

void cleanup() {
    for (int i = 0; i < planes_num; i++) {
        planes[i].alive = 0;
    }
    for (int i = 0; i < planes_num; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&starting);
    pthread_cond_destroy(&landing);
    free(thread_ids);
    free(planes);
}

void sighandler(int sig) {
    exit(0);
}

void wait_for_perm(struct plane *p, int tmp) {
    pthread_mutex_lock(&mtx);
    if (tmp == 1) { //start
        wait_to_start++;
        printf("plane %d wait for perm to start\n",p->id);
        while (!isfree) {
            pthread_cond_wait(&starting, &mtx);
        }
        isfree = 0;
        wait_to_start--;
    } else if (tmp == 0) { //land
        wait_to_land++;
        printf("plane %d wait for perm to land\n",p->id);
        while (!isfree || on_board == n) {
            pthread_cond_wait(&landing, &mtx);
        }
        isfree = 0;
        wait_to_land--;
    }
    pthread_mutex_unlock(&mtx);
    return;
}

void free_air() {
    if (on_board < k) {
        if (wait_to_land > 0) {
            pthread_cond_signal(&landing);
        } else {
            pthread_cond_signal(&starting);
        }
    } else {
        if (wait_to_start > 0) {
            pthread_cond_signal(&starting);
        } else if (on_board < n) {
            pthread_cond_signal(&landing);
        }
    }
}

void land(struct plane *p) {
    pthread_mutex_lock(&mtx);
    ++on_board;
    isfree = 1;
    free_air();
    printf("Plane %d landed, on board %d, want to start %d, want to land %d\n", p->id,on_board,wait_to_start,wait_to_land);
    pthread_mutex_unlock(&mtx);
}

void start(struct plane *p) {
    pthread_mutex_lock(&mtx);
    on_board--;
    isfree = 1;
    free_air();
    printf("Plane %d started, on board %d, want to start %d, want to land %d\n", p->id,on_board,wait_to_start,wait_to_land);
    pthread_mutex_unlock(&mtx);
}

void *thread_func(void *arg) {
    struct plane *p = arg;
    while (p->alive) {
        wait_for_perm(p, 0);
        land(p);
        usleep(10000);
        wait_for_perm(p, 1);
        start(p);
        usleep(10000);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("not enough args\n");
        exit(1);
    }
    signal(SIGINT, sighandler);
    atexit(cleanup);
    srand(time(NULL));;
    planes_num = atoi(argv[1]);
    n = atoi(argv[2]);
    k = atoi(argv[3]);
    thread_ids = malloc(planes_num * sizeof(pthread_t));
    planes = malloc(planes_num * sizeof(struct plane));

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&starting, NULL);
    pthread_cond_init(&landing, NULL);

    sigset_t mask;
    sigset_t old;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, &old);

    for (int i = 0; i < planes_num; i++) {
        planes[i].id = i;
        planes[i].alive = 1;
        if (pthread_create(&thread_ids[i], NULL, thread_func, &planes[i]) != 0)
            exit(1);
    }
    pthread_sigmask(SIG_SETMASK, &old, NULL);

    while (1) {
        pause();
    }
}