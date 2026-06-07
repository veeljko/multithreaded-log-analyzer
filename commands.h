#ifndef COMMANDS_H
#define COMMANDS_H

int isNoScanner(const char *input);
int isAdd(const char *input);
int isTop(const char *input);
int isWatch(const char *input);
int isStop(const char *input);
int isStats(const char *input);
int isLog(const char *fileName);
const char *getPath(const char *input);


#endif
