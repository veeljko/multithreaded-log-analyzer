#include "common.h"
#include "queue.h"
#include "reader.h"
#include "stats.h"

int getOffsetIndex(const char *path) {


    for (int i = 0; i < MAX_FILES; i++) {
        if (readFiles[i].used && strcmp(path, readFiles[i].path) == 0) {
            return i;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (!readFiles[i].used) {
            readFiles[i].used = 1;
            snprintf(readFiles[i].path, sizeof(readFiles[i].path), "%s", path);
            readFiles[i].offset = 0;


            pthread_mutex_lock(&statMutex);
            stats.cntFile++;
            pthread_mutex_unlock(&statMutex);

            return i;
        }
    }

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

int getEndOffsetAndLastSeq(const char *filePath, long long *endOut, int *seqOut) {
    FILE *file = fopen(filePath, "rb");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(file);
        return -1;
    }

    long end = ftell(file);

    if (end <= 0) {
        fclose(file);
        return 0; // prazan fajl
    }

    *endOut = end;

    while (end > 0) {
        if (fseek(file, end - 1, SEEK_SET) != 0) {
            perror("fseek");
            fclose(file);
            return -1;
        }

        int c = fgetc(file);

        if (c == '\n' || c == '\r') {
            end--;
        } else {
            break;
        }
    }

    if (end == 0) {
        fclose(file);
        return 0;
    }

    long start = end;

    while (start > 0) {
        if (fseek(file, start - 1, SEEK_SET) != 0) {
            perror("fseek");
            fclose(file);
            return -1;
        }

        int c = fgetc(file);

        if (c == '\n') {
            break;
        }

        start--;
    }

    long len = end - start;

    char line[LAST_LINE_LEN];

    if (len >= LAST_LINE_LEN) {
        len = LAST_LINE_LEN - 1;
    }

    if (fseek(file, start, SEEK_SET) != 0) {
        perror("fseek");
        fclose(file);
        return -1;
    }

    size_t readBytes = fread(line, 1, len, file);
    line[readBytes] = '\0';

    fclose(file);

    if (sscanf(line, "%d.", seqOut) == 1) {
        return 1;
    }

    return -1;
}

void readFromLastPosition(const char *filePath, long long offset, int seq) {
    FILE *file = fopen(filePath, "r");

    if (file == NULL) {
        perror("fopen");
        return;
    }

    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("fseek");
        fclose(file);
        return;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file) != NULL) {
        // sleep(2);
        struct logLine log;

        if (parseLogLine(line, &log)) {
            updateStats(&log);

            if (log.seqNo == seq) {
                break;
            }
        } else {
            printf("Neispravna log linija u fajlu %s: %s", filePath, line);
        }
    }

    fclose(file);
}

void* worker(void* args) {
    while (atomic_load(&running)) {
        char filePath[PATH_LEN];

        if (!takeProduct(filePath)) {
            break;
        }

        pthread_mutex_lock(&readFilesMutex);

        int offsetIndex = getOffsetIndex(filePath);

        if (offsetIndex == -1) {
            pthread_mutex_unlock(&readFilesMutex);
            printf("Nema mesta u readFiles za fajl: %s\n", filePath);
            continue;
        }

        long long oldOffset = readFiles[offsetIndex].offset;

        long long newOffset;
        int seq;

        int result = getEndOffsetAndLastSeq(filePath, &newOffset, &seq);

        if (result != 1) {
            pthread_mutex_unlock(&readFilesMutex);
            continue;
        }

        if (newOffset < oldOffset) {
            oldOffset = 0;
        }

        if (newOffset == oldOffset) {
            pthread_mutex_unlock(&readFilesMutex);
            continue;
        }

        readFiles[offsetIndex].offset = newOffset;

        pthread_mutex_unlock(&readFilesMutex);

        readFromLastPosition(filePath, oldOffset, seq);
    }

    return NULL;
}
