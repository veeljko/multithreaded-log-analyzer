#include "common.h"
#include "stats.h"
#include "watcher.h"

void* watch(void *args){

    while (atomic_load(&running) && atomic_load(&watching)){
        system("clear");
        printStats();
        sleep(1);
    }

    return NULL;
}
