#include "common.h"
#include "queue.h"

int addProduct(const char *path) {
    while (atomic_load(&running)) {
        if (sem_wait(&emptySlots) == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        perror("sem_wait emptySlots");
        return 0;
    }

    if (!atomic_load(&running)) {
        return 0;
    }

    pthread_mutex_lock(&productsMutex);

    snprintf(products[productTail], sizeof(products[productTail]), "%s", path);
    productTail = (productTail + 1) % MAX_PRODUCTS;

    pthread_mutex_unlock(&productsMutex);

    sem_post(&fullSlots);

    return 1;
}

int takeProduct(char *outPath) {
    while (atomic_load(&running)) {
        if (sem_wait(&fullSlots) == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        perror("sem_wait fullSlots");
        return 0;
    }

    if (!atomic_load(&running)) {
        return 0;
    }

    pthread_mutex_lock(&productsMutex);

    snprintf(outPath, PATH_LEN, "%s", products[productHead]);
    productHead = (productHead + 1) % MAX_PRODUCTS;

    pthread_mutex_unlock(&productsMutex);

    sem_post(&emptySlots);

    return 1;
}
