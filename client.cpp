#include <string>
#include <fstream>
#include <regex>
#include <iostream>
#include <windows.h>

struct Employee {
    int num; // идентификационный номер сотрудника
    char name[10]; // имя сотрудника
    double hours; // количество отработанных часов
};

int main(int argc, char *argv[]) {
    if (argc != 1) {
        std::cerr << "client.exe: Incorrect number of arguments." << std::endl;
        return 2;
    }
    std::string fileName = argv[0];

    std::cout << "client.exe: Filename: " << fileName << std::endl;

    int action = 2;
    HANDLE hPipe;
    hPipe = CreateFile("\\\\.\\pipe\\test_pipe",
                       /*GENERIC_READ | GENERIC_WRITE | */GENERIC_ALL,
                       0,
                       nullptr,
                       OPEN_EXISTING,
                       0,
                       nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cout << "client.exe: Could not open pipe (error " << GetLastError() << ")" << std::endl;
        return 1;
    }

    /*WriteFile(hPipe,
              "Hello Server!\n",
              12,   // = length of string + terminating '\0' !!!
              &dwWritten,
              nullptr);*/

    int key;
    BOOL hSend;
    DWORD dwSent;
    HANDLE hRead = OpenEvent(EVENT_MODIFY_STATE, false, "hRead_0");
    while (true) {
        std::cout << "client.exe: Print 0 to modify records, 1 to read, 2 to exit: " << std::endl;
        std::cin >> action;
        if (action == 0) {
            std::cout << "client.exe: Write" << std::endl;
        } else if (action == 1) {
            std::cout << "client.exe: Read" << std::endl;
            std::cout << "client.exe: Enter employee id: " << std::endl;
            std::cin >> key;
            std::string data = std::to_string(key);

            hSend = WriteFile(
                    hPipe,
                    data.c_str(),
                    data.size(),
                    &dwSent,
                    nullptr
            );
            if (hSend) {
                std::cout << "client.exe: Number of bytes sent: " << dwSent << std::endl;
            } else {
                std::cout << "client.exe: Failed to send data." << std::endl;
            }

            char* buffer = new char[1024];
            DWORD numBytesRead = 0;
            while(true) {
                BOOL hReceive = ReadFile(
                        hPipe,
                        buffer, // the data from the pipe will be put here
                        sizeof(buffer), // number of bytes allocated
                        &numBytesRead, // this will store number of bytes actually read
                        nullptr // not using overlapped IO
                );
                if (hReceive) {
                    buffer[numBytesRead + 1] = '\0'; // null terminate the string
                    std::cout << "client.exe: Number of bytes read: " << numBytesRead << std::endl;
                    std::cout << "client.exe: Id: " << buffer << std::endl;
                    break;
                }
            }


        } else {
            break;
        }
    }

    CloseHandle(hPipe);

    /*std::wcout << "Connecting to pipe..." << std::endl;
    // Open the named pipe
    // Most of these parameters aren't very relevant for pipes.
    HANDLE pipe = CreateFile(
            "\\\\.\\pipe\\my_pipe",
            GENERIC_READ, // only need read access
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
    );
    if (pipe == INVALID_HANDLE_VALUE) {
        std::wcout << "Failed to connect to pipe." << std::endl;
        // look up error code here using GetLastError()
        system("pause");
        return 1;
    }
    std::wcout << "Reading data from pipe..." << std::endl;
    // The read operation will block until there is data to read
    wchar_t buffer[128];
    DWORD numBytesRead = 0;
    BOOL result = ReadFile(
            pipe,
            buffer, // the data from the pipe will be put here
            127 * sizeof(wchar_t), // number of bytes allocated
            &numBytesRead, // this will store number of bytes actually read
            NULL // not using overlapped IO
    );
    if (result) {
        buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
        std::wcout << "Number of bytes read: " << numBytesRead << std::endl;
        std::wcout << "Message: " << buffer << std::endl;
    } else {
        std::wcout << "Failed to read data from the pipe." << std::endl;
    }
    // Close our pipe handle
    CloseHandle(pipe);
    std::wcout << "Done." << std::endl;
    system("pause");*/


    return 0;
}