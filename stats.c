#include "common.h"
#include "stats.h"

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
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (msgCount[i].used && strcmp(msg, msgCount[i].msg) == 0) {
            msgCount[i].cnt++;
            return;
        }
    }

    for (int i = 0; i < MAX_MESSAGES; i++) {
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
    for (int i=0;i<MAX_MESSAGES;i++){
        if (!msgCount[i].used) continue;
        for (int j=i+1;j<MAX_MESSAGES;j++){
            if (!msgCount[j].used) continue;

            if (msgCount[i].cnt < msgCount[j].cnt) swapMessages(i, j);
        }
    }
}

void topN(int n){
    if (n > MAX_MESSAGES) n = MAX_MESSAGES;
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
