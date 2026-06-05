#include "common.h"
#include "commands.h"

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

int isStats(const char *input){
    return strcmp(input, "stats") == 0;
}

int isLog(const char *fileName) {
    size_t len = strlen(fileName);

    if (len < 4) {
        return 0;
    }

    return strcmp(fileName + len - 4, ".log") == 0 || strcmp(fileName + len - 4, ".txt") == 0;
}

const char *getPath(const char *input) {
    return input + 4;
}
