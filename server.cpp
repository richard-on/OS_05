#include <iostream>
#include <string>
#include <regex>

#include "server.h"

void printRecords() {
    binaryFile.seekg(0, std::ios::beg);
    int recordsNum;
    binaryFile.read((char*)&recordsNum, sizeof(int));
    Employee e{};
    std::cout << "server.exe: Printing records: " << std::endl;
    for (int i = 0; i < recordsNum; ++i) {
        binaryFile.read((char*)&e, sizeof(e));
        std::cout << e.num << " " << e.name << " " << e.hours << std::endl;
    }
}

bool findRecord(Employee &e, int num, DWORD &pos) {
    EnterCriticalSection(&cs);
    binaryFile.seekg(0, std::ios::beg);
    int recordNum;
    binaryFile.read((char*)&recordNum, sizeof(int));

    for (pos = 0; pos < recordNum; ++pos) {
        binaryFile.read((char*)&e, sizeof(Employee));
        if (e.num == num)
            break;
    }
    LeaveCriticalSection(&cs);

    if (pos == recordNum){
        return false;
    }
    return true;
}

void readFromFile(Employee &e, DWORD pos) {
    EnterCriticalSection(&cs);
    binaryFile.seekp(sizeof(int) + pos*sizeof(e), std::ios::beg);
    binaryFile.read((char*)&e, sizeof(e));
    LeaveCriticalSection(&cs);
}

void writeToFile(Employee e, DWORD pos) {
    EnterCriticalSection(&cs);
    binaryFile.seekp(sizeof(int) + pos*sizeof(e), std::ios::beg);
    binaryFile.write((char*)&e, sizeof(e));
    LeaveCriticalSection(&cs);
}

DWORD WINAPI server(LPVOID param) {
    PipeArray p = *(PipeArray*)param;
    BOOL hConnect = ConnectNamedPipe(p.hPipe, nullptr);
    if (!hConnect) {
        std::cerr << "server.exe: Could not connect to pipe " << p.idx << " (error " << GetLastError() << ")" << std::endl;
        CloseHandle(p.hPipe);
        return 1;
    }
    DWORD dwSent = 0;
    DWORD dwRead = 0;
    DWORD pos = 0;
    Employee e{};
    Request r{};

    while (r.type != 2) {
        ReadFile(p.hPipe, &r, sizeof(r), &dwRead, nullptr);

        if (r.type == 0) {
            if (!findRecord(e, r.key, pos)) {
                std::cout << "server.exe: Unable to find record with id " << r.key << std::endl;
                e.num = -1;
                e.hours = -1;
            } else {
                curReaders[pos]++;
                WaitForSingleObject(hWriteAvail[pos], INFINITE);
                ResetEvent(hReadAvail[pos]);
                readFromFile(e, pos);
                std::cout << "server.exe: Found record: " << e.num << " " << e.name << " " << e.hours << std::endl;
            }

            if (!WriteFile(p.hPipe, &e, sizeof(e), &dwSent, nullptr)) {
                std::cerr << "server.exe: Failed to send data." << std::endl;
            }
            curReaders[pos]--;
            if (!curReaders[pos]) {
                SetEvent(hReadAvail[pos]);
            }

        } else if (r.type == 1) {
            if (!findRecord(e, r.key, pos)) {
                std::cout << "server.exe: Unable to find record with id " << r.key << std::endl;
                e.num = -1;
                e.hours = -1;
            } else {
                WaitForSingleObject(hReadAvail[pos], INFINITE);
                WaitForSingleObject(hWriteAvail[pos], INFINITE);
                ResetEvent(hWriteAvail[pos]);
                readFromFile(e, pos);
                std::cout << "server.exe: Found record: " << e.num << " " << e.name << " " << e.hours << std::endl;
            }

            if (!WriteFile(p.hPipe, &e, sizeof(e), &dwSent, nullptr)) {
                std::cerr << "server.exe: Failed to send data." << std::endl;
            }

            ReadFile(p.hPipe, &e, sizeof(e), &dwRead, nullptr);
            writeToFile(e, pos);
            SetEvent(hWriteAvail[pos]);
        }
    }

    DisconnectNamedPipe(p.hPipe);
    return 0;
}