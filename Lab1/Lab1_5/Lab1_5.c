#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>

typedef enum {
    EMPTY,
    WOMEN_ONLY,
    MEN_ONLY,
    INVALID_N,
    WRONG_NUMBER_OF_ARGS,
    SUCCESS
} CODES;

typedef struct {
    CODES status;
    int count;
    int max_people;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}bathroom_t;

bathroom_t bathroom;

void bathroom_init(int max_people) {
    bathroom.status = EMPTY;
    bathroom.count = 0;
    bathroom.max_people = max_people;
    pthread_mutex_init(&bathroom.mutex, NULL);
    pthread_cond_init(&bathroom.cond, NULL);
}

void bathroom_destroy() {
    pthread_mutex_destroy(&bathroom.mutex);
    pthread_cond_destroy(&bathroom.cond);
}

void woman_wants_to_enter() {
    pthread_mutex_lock(&bathroom.mutex);
    while(bathroom.status == MEN_ONLY || bathroom.count >= bathroom.max_people) {
        pthread_cond_wait(&bathroom.cond, &bathroom.mutex);
    }

    if (bathroom.status == EMPTY) {
        bathroom.status = WOMEN_ONLY;
    }
    bathroom.count++;
    printf("Woman entered. Now in bathroom: %d women\n", bathroom.count);
    pthread_mutex_unlock(&bathroom.mutex);
}

void man_wants_to_enter() {
    pthread_mutex_lock(&bathroom.mutex);
    while(bathroom.status == WOMEN_ONLY || bathroom.count >= bathroom.max_people) {
        pthread_cond_wait(&bathroom.cond, &bathroom.mutex);
    }

    if (bathroom.status == EMPTY) {
        bathroom.status = MEN_ONLY;
    }
    bathroom.count++;
    printf("Man entered. Now in bathroom: %d men\n", bathroom.count);

    pthread_mutex_unlock(&bathroom.mutex);
}

void woman_leaves() {
    pthread_mutex_lock(&bathroom.mutex);
    if (bathroom.status == MEN_ONLY) {
        printf("Now men are in bathroom\n");
        pthread_mutex_unlock(&bathroom.mutex);
        return;
    }
    if (bathroom.status == EMPTY) {
        printf("Now is empty in bathroom\n");
        pthread_mutex_unlock(&bathroom.mutex);
        return;
    }
    bathroom.count--;
    printf("Woman left. Remained in bathroom: %d women\n", bathroom.count);

    if (bathroom.count == 0) {
        bathroom.status = EMPTY;
    }

    pthread_cond_broadcast(&bathroom.cond);
    pthread_mutex_unlock(&bathroom.mutex);
}

void man_leaves() {
    pthread_mutex_lock(&bathroom.mutex);
    if (bathroom.status == WOMEN_ONLY) {
        printf("Now women are in bathroom\n");
        pthread_mutex_unlock(&bathroom.mutex);
    }
    if (bathroom.status == EMPTY) {
        printf("Now is empty in bathroom\n");
        pthread_mutex_unlock(&bathroom.mutex);
    }
    bathroom.count--;
    printf("Man left. Remained in bathroom: %d men\n", bathroom.count);

    if (bathroom.count == 0) {
        bathroom.status = EMPTY;
    }

    pthread_cond_broadcast(&bathroom.cond);
    pthread_mutex_unlock(&bathroom.mutex);
}

void* woman_thread(void* arg) {
    woman_wants_to_enter();
    sleep(1);
    woman_leaves();
    return NULL;
}

void* man_thread(void* arg) {
    man_wants_to_enter();
    sleep(1);
    man_leaves();
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Wrong number of args\n");
        return WRONG_NUMBER_OF_ARGS;
    }
    for (int i = 0; i < 8; i++) {
        if (argv[1][i] == '\0') {
            break;
        }
        if (!isdigit(argv[1][i])) {
            printf("N must be num and length of N must be less than 9\n");
            return INVALID_N;
        }
    }
    bathroom_init(atoi(argv[1]));

    pthread_t threads[10];

    pthread_create(&threads[0], NULL, woman_thread, NULL);
    pthread_create(&threads[1], NULL, man_thread, NULL);
    pthread_create(&threads[2], NULL, woman_thread, NULL);
    pthread_create(&threads[3], NULL, man_thread, NULL);
    pthread_create(&threads[4], NULL, woman_thread, NULL);
    pthread_create(&threads[5], NULL, man_thread, NULL);
    pthread_create(&threads[6], NULL, woman_thread, NULL);
    pthread_create(&threads[7], NULL, man_thread, NULL);
    pthread_create(&threads[8], NULL, woman_thread, NULL);
    pthread_create(&threads[9], NULL, man_thread, NULL);

    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    bathroom_destroy();
    return SUCCESS;
}