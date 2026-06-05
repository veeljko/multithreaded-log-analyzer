#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <limits.h>
#include <stdatomic.h>
#include <errno.h>

#define MAX_MESSAGES 10000
#define WORKER_CNT 6
#define PRODUCER_CNT 100
#define MAX_FILES 200
#define MAX_PRODUCTS 200
#define TIMESTAMP_LEN 100
#define LEVEL_LEN 20
#define MESSAGE_LEN 500
#define LAST_LINE_LEN 1024
#define PATH_LEN PATH_MAX

enum LOG_TYPE { ERROR, WARNING, INFO };

struct logFile {
    char path[PATH_LEN];
    time_t modifyTime;
    int used;
};

struct readFile{
    char path[PATH_LEN];
    long long offset;
    int used;
};

struct stats{
    int cntFile;
    int cntLines;
    int cntError;
    int cntWarning;
    int cntInfo;
};

struct logLine {
    int seqNo;
    char timestamp[TIMESTAMP_LEN];
    char level[LEVEL_LEN];
    char message[MESSAGE_LEN];
};

struct msgCount {
    char msg[MESSAGE_LEN];
    int cnt;
    int used;
};

extern pthread_t producers[PRODUCER_CNT];
extern int prodInd;

extern char products[MAX_PRODUCTS][PATH_LEN];
extern int productHead;
extern int productTail;

extern pthread_mutex_t scannedFilesMutex;
extern pthread_mutex_t productsMutex;
extern pthread_mutex_t readFilesMutex;
extern pthread_mutex_t statMutex;

extern sem_t emptySlots;
extern sem_t fullSlots;

extern struct logFile scannedFiles[MAX_FILES];
extern struct readFile readFiles[MAX_FILES];
extern struct stats stats;
extern struct msgCount msgCount[MAX_MESSAGES];

extern atomic_int running;
extern atomic_int watching;

#endif
