#include "common.h"
#include "commands.h"
#include "queue.h"
#include "scanner.h"

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

    while (atomic_load(&running)) {
        DIR *dir = opendir(path);

        if (dir == NULL) {
            printf("Ne mogu da otvorim direktorijum: %s\n", path);
            sleep(5);
            continue;
        }

        struct dirent *entry;
        while (atomic_load(&running)){
            while ((entry = readdir(dir)) != NULL) {
                if (isLog(entry->d_name)) {
                    // sleep(2);
                    char fullPath[PATH_LEN];

                    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

                    struct stat info;
                    if (stat(fullPath, &info) == -1) {
                        perror("stat");
                        continue;
                    }

                    if (!S_ISREG(info.st_mode)) {
                        continue;
                    }

                    int status = checkLogFile(fullPath, scannedFiles);

                    if (status == 1) {
                        // printf("Novi ili izmenjen log fajl: %s\n", fullPath);
                        addProduct(fullPath);
                    }
                }
            }
            sleep(5);
            rewinddir(dir);
        }

        closedir(dir);

    }

    free(path);
    return NULL;
}
