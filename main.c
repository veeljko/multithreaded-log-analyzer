#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>


#define WORKER_CNT 6
#define PRODUCER_CNT 100
#define MAX_FILES 200
#define MAX_PRODUCTS 200
#define PATH_LEN 200
#define TIMESTAMP_LEN 100
#define LEVEL_LEN 20
#define MESSAGE_LEN 500

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

pthread_t producers[PRODUCER_CNT];
int prodInd = 0;

char products[MAX_PRODUCTS][PATH_LEN];
int productsInd = 0;

pthread_mutex_t scannedFilesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t productsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readFilesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statMutex = PTHREAD_MUTEX_INITIALIZER;

sem_t sem;

struct logFile scannedFiles[MAX_FILES] = {0};
struct readFile readFiles[MAX_FILES] = {0};
struct stats stats = {0};
struct msgCount msgCount[MAX_FILES] = {0};


int isAdd(const char *input) {
    return strncmp(input, "add ", 4) == 0;
}

int isTop(const char *input){
    return strncmp(input, "top ", 4) == 0;
}

int isWatch(const char *input){
    return strcmp(input, "watch") == 0;
}

int isStop(const char *input){
    return strcmp(input, "stop") == 0;
}

int isLog(const char *fileName) {
    size_t len = strlen(fileName);

    if (len < 4) {
        return 0;
    }

    return strcmp(fileName + len - 4, ".log") == 0;
}

const char *getPath(const char *input) {
    return input + 4;
}

void addProduct(const char *path) {
    pthread_mutex_lock(&productsMutex);

    if (productsInd < MAX_PRODUCTS) {
        snprintf(products[productsInd], sizeof(products[productsInd]), "%s", path);
        productsInd++;

        sem_post(&sem);
    } else {
        printf("products niz je pun.\n");
    }

    pthread_mutex_unlock(&productsMutex);
}

int checkLogFile(const char *path, struct logFile scannedFiles[]) {
    struct stat info;

    if (stat(path, &info) == -1) {
        perror("stat");
        return -1;
    }

    pthread_mutex_lock(&scannedFilesMutex);

    for (int i = 0; i < MAX_FILES; i++) {
        if (!scannedFiles[i].used) {
            continue;
        }

        if (strcmp(path, scannedFiles[i].path) == 0) {
            if (info.st_mtime == scannedFiles[i].modifyTime) {
                pthread_mutex_unlock(&scannedFilesMutex);
                return 0;
            }

            scannedFiles[i].modifyTime = info.st_mtime;

            pthread_mutex_unlock(&scannedFilesMutex);
            return 1;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!scannedFiles[i].used) {
            scannedFiles[i].used = 1;
            snprintf(scannedFiles[i].path, sizeof(scannedFiles[i].path), "%s", path);
            scannedFiles[i].modifyTime = info.st_mtime;

            pthread_mutex_unlock(&scannedFilesMutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&scannedFilesMutex);
    return -1;
}
void* scanDir(void *arg) {
    char *path = (char *) arg;

    while (1) {
        DIR *dir = opendir(path);

        if (dir == NULL) {
            printf("Ne mogu da otvorim direktorijum: %s\n", path);
            sleep(5);
            continue;
        }

        struct dirent *entry;
        while (1){
            while ((entry = readdir(dir)) != NULL) {
                if (isLog(entry->d_name)) {
                    char fullPath[PATH_LEN];

                    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

                    int status = checkLogFile(fullPath, scannedFiles);

                    if (status == 1) {
                        printf("Novi ili izmenjen log fajl: %s\n", fullPath);
                        addProduct(fullPath);
                    }
                }
            }
            sleep(5);
            rewinddir(dir);
        }

        closedir(dir);

        printf("Zavrseno skeniranje direktorijuma %s. Cekam 5 sekundi...\n", path);
        sleep(5);
    }

    free(path);
    return NULL;
}

int getStartLine(const char *path) {
    pthread_mutex_lock(&readFilesMutex);

    for (int i = 0; i < MAX_FILES; i++) {
        if (readFiles[i].used && strcmp(path, readFiles[i].path) == 0) {
            pthread_mutex_unlock(&readFilesMutex);
            return i;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!readFiles[i].used) {
            readFiles[i].used = 1;
            snprintf(readFiles[i].path, sizeof(readFiles[i].path), "%s", path);
            readFiles[i].offset = 0;

            pthread_mutex_unlock(&readFilesMutex);

            pthread_mutex_lock(&statMutex);
            stats.cntFile++;
            pthread_mutex_unlock(&statMutex);

            return i;
        }
    }

    pthread_mutex_unlock(&readFilesMutex);
    return -1;
}

int parseLogLine(const char *line, struct logLine *log) {
    int result = sscanf(
        line,
        "%d. %99s severity=\"%19[^\"]\" %499[^\n]",
        &log->seqNo,
        log->timestamp,
        log->level,
        log->message
    );

    return result == 4;
}

void printStats() {
    pthread_mutex_lock(&statMutex);

    printf("\n=== STATS ===\n");
    printf("Broj fajlova: %d\n", stats.cntFile);
    printf("Broj linija: %d\n", stats.cntLines);
    printf("ERROR: %d\n", stats.cntError);
    printf("WARNING: %d\n", stats.cntWarning);
    printf("INFO: %d\n", stats.cntInfo);
    printf("=============\n\n");

    pthread_mutex_unlock(&statMutex);
}

void updateMessages(const char *msg) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (msgCount[i].used && strcmp(msg, msgCount[i].msg) == 0) {
            msgCount[i].cnt++;
            return;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!msgCount[i].used) {
            msgCount[i].used = 1;
            snprintf(msgCount[i].msg, sizeof(msgCount[i].msg), "%s", msg);
            msgCount[i].cnt = 1;
            return;
        }
    }
}

void swapMessages(int i, int j){
    struct msgCount tmp;
    tmp.cnt = msgCount[i].cnt;
    strcpy(tmp.msg, msgCount[i].msg);

    msgCount[i].cnt = msgCount[j].cnt;
    strcpy(msgCount[i].msg, msgCount[j].msg);

    msgCount[j].cnt = tmp.cnt;
    strcpy(msgCount[j].msg, tmp.msg);
}

void sortMessagesByCount(){
    for (int i=0;i<MAX_FILES;i++){
        if (!msgCount[i].used) continue;
        for (int j=i+1;j<MAX_FILES;j++){
            if (!msgCount[j].used) continue;

            if (msgCount[i].cnt < msgCount[j].cnt) swapMessages(i, j);
        }
    }
}

void topN(int n){
    pthread_mutex_lock(&statMutex);

    sortMessagesByCount();
    for (int i=0;i<n;i++){
        if (!msgCount[i].used) break;
        printf("%d %s (%d)\n", i+1, msgCount[i].msg, msgCount[i].cnt);
    }
    pthread_mutex_unlock(&statMutex);
}

void updateStats(const struct logLine *log) {
    pthread_mutex_lock(&statMutex);

    stats.cntLines++;

    if (strcmp(log->level, "ERROR") == 0) {
        stats.cntError++;
    } else if (strcmp(log->level, "WARNING") == 0) {
        stats.cntWarning++;
    } else if (strcmp(log->level, "INFO") == 0) {
        stats.cntInfo++;
    }

    updateMessages(log->message);

    pthread_mutex_unlock(&statMutex);
}

void readFromLastPosition(const char *filePath, int index) {
    if (index < 0) {
        printf("Nema mesta u readFiles nizu za fajl: %s\n", filePath);
        return;
    }

    FILE *file = fopen(filePath, "r");

    if (file == NULL) {
        perror("fopen");
        return;
    }

    pthread_mutex_lock(&readFilesMutex);
    long long offset = readFiles[index].offset;
    pthread_mutex_unlock(&readFilesMutex);

    fseek(file, offset, SEEK_SET);

    char line[1024];

    while (fgets(line, sizeof(line), file) != NULL) {
        struct logLine log;

        if (parseLogLine(line, &log)) {
            updateStats(&log);
        } else {
            printf("Neispravna log linija u fajlu %s: %s", filePath, line);
        }
    }

    long long newOffset = ftell(file);

    pthread_mutex_lock(&readFilesMutex);
    readFiles[index].offset = newOffset;
    pthread_mutex_unlock(&readFilesMutex);

    fclose(file);
}

void* worker(void* args) {
    while (1) {
        sem_wait(&sem);

        pthread_mutex_lock(&productsMutex);

        productsInd--;

        char filePath[PATH_LEN];
        snprintf(filePath, sizeof(filePath), "%s", products[productsInd]);

        pthread_mutex_unlock(&productsMutex);

        printf("Worker obradjuje fajl: %s\n", filePath);

        // Ovde kasnije ide prava obrada fajla.
        int offsetIndex = getStartLine(filePath);
        readFromLastPosition(filePath, offsetIndex);

    }

    return NULL;
}

void* watch(void *args){
    while (1){
        system("clear");
        printStats();
        sleep(1);
    }
}

int main() {
    char input[PATH_LEN];

    pthread_t workers[WORKER_CNT];
    pthread_t watchThread;
    sem_init(&sem, 0, 0);
    for (int i = 0; i < WORKER_CNT; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }

    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
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
        else if (isWatch(input)){
            pthread_create(&watchThread, NULL, watch, NULL);
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
            break;
        }
    }

    for (int i = 0; i < prodInd; i++) {
        pthread_join(producers[i], NULL);
    }
    pthread_join(watchThread, NULL);

    pthread_mutex_destroy(&statMutex);
    pthread_mutex_destroy(&scannedFilesMutex);
    pthread_mutex_destroy(&readFilesMutex);
    return 0;
}
