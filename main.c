#include "common.h"
#include "commands.h"
#include "reader.h"
#include "scanner.h"
#include "stats.h"
#include "watcher.h"

int main() {
    char input[PATH_LEN];

    pthread_t workers[WORKER_CNT];
    pthread_t watchThread;
    sem_init(&emptySlots, 0, MAX_PRODUCTS);
    sem_init(&fullSlots, 0, 0);
    for (int i = 0; i < WORKER_CNT; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }


    int watchStarted = 0;
    while (atomic_load(&running)) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            atomic_store(&running, 0);

            for (int i = 0; i < WORKER_CNT; i++) {
                sem_post(&fullSlots);
            }

            for (int i = 0; i < prodInd; i++) {
                sem_post(&emptySlots);
            }

            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (isAdd(input)) {
            if (prodInd >= PRODUCER_CNT) {
                printf("Maksimalan broj producer thread-ova dostignut.\n");
                continue;
            }

            const char *pathStart = getPath(input);

            char *pathCopy = malloc(strlen(pathStart) + 1);
            if (pathCopy == NULL) {
                perror("malloc");
                continue;
            }

            strcpy(pathCopy, pathStart);

            pthread_create(&producers[prodInd], NULL, scanDir, (void *)pathCopy);
            prodInd++;
        }
        else if (isWatch(input)) {
            if (!watchStarted) {
                atomic_store(&watching, 1);
                pthread_create(&watchThread, NULL, watch, NULL);
                watchStarted = 1;

                while (atomic_load(&running)) {
                    if (fgets(input, sizeof(input), stdin) == NULL) {
                        clearerr(stdin);
                        break;
                    }
                }

                atomic_store(&watching, 0);
                pthread_join(watchThread, NULL);
                watchStarted = 0;
                system("clear");
            }
        }
        else if (isTop(input)){
            int n;

            if (sscanf(input, "top %d", &n) == 1) {
                //printf("N = %d\n", n);
            } else {
                printf("Neispravna komanda.\n");
                continue;
            }
            topN(n);
        }
        else if (isStop(input)){
            atomic_store(&running, 0);

            for (int i = 0; i < WORKER_CNT; i++) {
                sem_post(&fullSlots);
            }

            for (int i = 0; i < prodInd; i++) {
                sem_post(&emptySlots);
            }
        }
        else if (isStats(input)){
            printStats();
        }
        else if (isNoScanner(input)){
            printf("Broj pokrenutih scanner niti je: %d\n", getCntScanner());
        }
    }


    for (int i = 0; i < WORKER_CNT; i++) {
        pthread_join(workers[i], NULL);
    }
    for (int i = 0; i < prodInd; i++) {
        pthread_join(producers[i], NULL);
    }
    if (watchStarted) {
        pthread_join(watchThread, NULL);
    };

    pthread_mutex_destroy(&statMutex);
    pthread_mutex_destroy(&scannedFilesMutex);
    pthread_mutex_destroy(&readFilesMutex);
    pthread_mutex_destroy(&cntMutex);
    return 0;
}
