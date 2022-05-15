#ifndef OS_05_SERVER_H
#define OS_05_SERVER_H

#include <windows.h>
#include <fstream>

struct PipeArray {
    HANDLE hPipe;
    int idx;
};

struct Employee {
    int num;
    char name[10];
    double hours;
};

struct Request {
    int type;
    int key;
};

extern std::fstream binaryFile;
extern CRITICAL_SECTION cs;
extern HANDLE* hWriteAvail;
extern HANDLE* hReadAvail;
extern int* curReaders;

void printRecords();
void writeToFile(Employee e, DWORD pos);
void readFromFile(Employee &e, DWORD pos);
bool findRecord(Employee &e, int num, DWORD &pos);

DWORD WINAPI server(LPVOID param);

#endif //OS_05_SERVER_H
