#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <zconf.h>

typedef struct plane {
    int id;
    int time;
    int alive;
    int waiting;
} plane;


pthread_mutex_t mtx;
pthread_cond_t starting;
pthread_cond_t landing;
int planes_num;
int n;
int k;
int wait_to_start = 0;
int wait_to_land = 0;
int isfree = 1;
int on_board = 0;
pthread_t *thread_ids;
struct plane *planes;
int *alive;

void cleanup() {
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&starting);
    pthread_cond_destroy(&landing);
    free(thread_ids);
    free(planes);
    free(alive);
}

void wait_for_perm(struct plane *p, int tmp) {
    pthread_mutex_lock(&mtx);
    if (tmp == 1) { //start
        while (!isfree) {
            if (!p->waiting) {
                p->waiting = 1;
                wait_to_start++;
            }
            pthread_cond_wait(&starting, &mtx);
        }
        isfree = 0;
        p->waiting = 0;
        wait_to_start--;
    } else if (tmp == 0) { //land
        while (!isfree || on_board == k) {
            pthread_cond_wait(&landing, &mtx);
            if (!p->waiting) {
                p->waiting = 1;
                ++wait_to_land;
            }
        }
        isfree = 0;
        p->waiting = 0;
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
    pthread_mutex_unlock(&mtx);
}

void start(struct plane *p) {
    pthread_mutex_lock(&mtx);
    on_board--;
    isfree = 1;
    free_air();
    pthread_mutex_unlock(&mtx);
}

void *thread_func(void *arg) {
    struct plane *p = arg;
    while (p->alive) {
        wait_for_perm(p, 0);
        land(p);
        printf("Plane %d landed\n", p->id);

        usleep(p->time);

        wait_for_perm(p, 1);
        start(p);
        printf("Plane %d started\n", p->id);
        usleep(p->time);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("not enough args\n");
        exit(1);
    }
    atexit(cleanup);
    srand(time(NULL));
    int maxs = 10000;
    planes_num = atoi(argv[1]);
    n = atoi(argv[2]);
    k = atoi(argv[3]);
    thread_ids = malloc(planes_num * sizeof(pthread_t));
    alive = malloc(planes_num * sizeof(int));
    planes = malloc(planes_num * sizeof(struct plane));

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&starting, NULL);
    pthread_cond_init(&landing, NULL);

    for (int i = 0; i < planes_num; i++)
        alive[i] = 1;

    for (int i = 0; i < planes_num - 1; i++) {
        planes[i].id = i;
        planes[i].time = rand() % maxs;
        planes[i].alive = alive[i];
        planes[i].waiting = 0;
        pthread_create(&thread_ids[i], NULL, thread_func, &planes[i]);
    }

    pause();

    for (int i = 0; i < planes_num; i++) {
        alive[i] = 0;
        pthread_join(thread_ids[i], NULL);
    }



    return 0;
}