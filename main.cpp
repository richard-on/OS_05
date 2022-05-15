#include <iostream>
#include <string>
#include <regex>

#include "server.h"

std::fstream binaryFile;
CRITICAL_SECTION cs;
HANDLE* hWriteAvail;
HANDLE* hReadAvail;
int* curReaders;

int main() {
    const std::string client = "client.exe";

    std::string fileName;
    int procNum, recordsNum;

    std::cout << "server.exe: Enter file name: " << std::endl;
    std::cin >> fileName;

    const std::regex fileNameRegex(R"(^(?:[a-z]:)?[\/\\]{0,2}(?:[.\/\\ ](?![.\/\\\n])|[^<>:"|?*.\/\\ \n])+$)");
    if(!std::regex_match(fileName, fileNameRegex)){
        std::cerr << "server.exe: Incorrect filename/filepath." << std::endl;
        return 1;
    }

    binaryFile.open(fileName, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
    if(!binaryFile) {
        std::cerr << "server.exe: Could not open file (error " << GetLastError() << ")" << std::endl;
        return 1;
    }

    std::cout << "server.exe: Enter records number" << std::endl;
    std::cin >> recordsNum;
    binaryFile.write((char*)&recordsNum, sizeof(int));

    hWriteAvail = new HANDLE[recordsNum];
    hReadAvail = new HANDLE[recordsNum];
    curReaders = new int[recordsNum]{0};

    std::cout << "server.exe: Enter employee data" << std::endl;
    for (int i = 0; i < recordsNum; i++) {
        hWriteAvail[i] = CreateEvent(nullptr, true, true, nullptr);
        hReadAvail[i] = CreateEvent(nullptr, true, true, nullptr);

        Employee e{};
        std::cin >> e.num >> e.name >> e.hours;
        binaryFile.write((char*)&e, sizeof(e));
    }
    if(!binaryFile.good()) {
        std::cerr << "server.exe: Error occurred at writing time." << std::endl;
        return 1;
    }

    printRecords();

    std::cout << "server.exe: Enter number of client processes: " << std::endl;
    std::cin >> procNum;
    if(procNum < 1) {
        std::cerr << "server.exe: Incorrect records number." << std::endl;
        return 1;
    }

    auto* si = new STARTUPINFO[procNum];
    auto* pi = new PROCESS_INFORMATION[procNum];
    auto* pipeArray = new PipeArray[procNum];
    auto* hServer = new HANDLE[procNum];
    auto* serverID = new DWORD[procNum];
    InitializeCriticalSection(&cs);

    for (int i = 0; i < procNum; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        pipeArray[i].idx = i;
        std::string pipeName = R"(\\.\pipe\pipe_)" + std::to_string(i);
        pipeArray[i].hPipe = CreateNamedPipe(pipeName.c_str(),
                                             PIPE_ACCESS_DUPLEX,
                                             PIPE_TYPE_BYTE,
                                             1,
                                             0,
                                             0,
                                             0,
                                             nullptr);

        if (pipeArray[i].hPipe == nullptr || pipeArray[i].hPipe == INVALID_HANDLE_VALUE) {
            std::cerr << "server.exe: Could not create pipe " << i << " (error " << GetLastError() << ")" << std::endl;
            return 1;
        }

        hServer[i] = CreateThread(nullptr,
                                  0,
                                  server,
                                  (void*)&pipeArray[i],
                                  0,
                                  serverID + i);
        if (hServer[i] == nullptr || hServer[i] == INVALID_HANDLE_VALUE) {
            std::cerr << "server.exe: Could not create server " << i << " (error " << GetLastError() << ")" << std::endl;
            return 1;
        }

        std::string cmdLine = fileName + " " + pipeName;
        if (!CreateProcess(client.c_str(),
                           const_cast<char*>(cmdLine.c_str()),
                           nullptr,
                           nullptr,
                           false,
                           CREATE_NEW_CONSOLE,
                           nullptr,
                           nullptr,
                           &si[i],
                           &pi[i])
                ) {
            std::cerr << "server.exe: Could not create process (error " << GetLastError() << ")" << std::endl;
            return 1;
        }

    }

    WaitForMultipleObjects(procNum, &pi->hProcess, true, INFINITE);
    printRecords();

    int action;
    std::cout << "server.exe: All clients exited. Type '0' to stop servers" << std::endl;
    std::cin >> action;
    while (action != 0) {
        std::cin >> action;
    }

    binaryFile.close();
    CloseHandle(hServer);

    delete[] si;
    delete[] pi;
    delete[] pipeArray;
    delete[] hServer;
    delete[] serverID;

    return 0;
}