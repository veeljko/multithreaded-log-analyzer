#ifndef SCANNER_H
#define SCANNER_H

int checkLogFile(const char *path, struct logFile scannedFiles[]);
void* scanDir(void *arg);

#endif
