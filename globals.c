#include "common.h"

pthread_t producers[PRODUCER_CNT];
int prodInd = 0;

char products[MAX_PRODUCTS][PATH_LEN];
int productHead = 0;
int productTail = 0;

pthread_mutex_t scannedFilesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t productsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readFilesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statMutex = PTHREAD_MUTEX_INITIALIZER;

sem_t emptySlots;
sem_t fullSlots;

struct logFile scannedFiles[MAX_FILES] = {0};
struct readFile readFiles[MAX_FILES] = {0};
struct stats stats = {0};
struct msgCount msgCount[MAX_MESSAGES] = {0};

atomic_int running = 1;
atomic_int watching = 0;
