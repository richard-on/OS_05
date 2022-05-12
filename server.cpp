#include <iostream>
#include <vector>
#include <windows.h>
#include <string>
#include <fstream>
#include <regex>
#include <iomanip>

struct Employee {
    int num; // идентификационный номер сотрудника
    char name[10]; // имя сотрудника
    double hours; // количество отработанных часов
};

/*bool createProcess(const std::string& applicationName, const std::string& cmdArgs) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcess(const_cast<char*>(applicationName.c_str()),
                       const_cast<char*>(cmdArgs.c_str()),
                       nullptr,
                       nullptr,
                       FALSE,
                       0,
                       nullptr,
                       nullptr,
                       &si,
                       &pi)
            ) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}*/

int main() {
    const std::string client = "client.exe";

    std::string fileName;
    int procNum;

    std::cout << "server.exe: Enter file name: " << std::endl;
    std::cin >> fileName;

    const std::regex fileNameRegex(R"(^(?:[a-z]:)?[\/\\]{0,2}(?:[.\/\\ ](?![.\/\\\n])|[^<>:"|?*.\/\\ \n])+$)");
    if(!std::regex_match(fileName, fileNameRegex)){
        std::cerr << "server.exe: Incorrect filename/filepath." << std::endl;
        return 1;
    }

    std::ofstream f(fileName);
    f.close();

    std::fstream binaryFile(fileName, std::ios::binary | std::ios::out);
    if(!binaryFile) {
        std::cerr << "server.exe: Could not open file (error " << GetLastError() << ")" << std::endl;
        return 1;
    }

    std::cout << "server.exe: Enter employee data" << std::endl;
    int action = 0;
    while (true) {
        std::cout << "server.exe: Print 0 to add next record or 1 to exit: " << std::endl;
        std::cin >> action;
        if (action != 0) {
            break;
        } else {
            std::cout << "server.exe: Enter next record: " <<  std::endl;
            Employee employee{};
            std::cin >> employee.num >> employee.name >> employee.hours;
            binaryFile.write((char*)&employee, sizeof(Employee));
            std::cout << "server.exe: Added record" << std::endl;
        }
    }
    binaryFile.close();
    if(!binaryFile.good()) {
        std::cerr << "server.exe: Error occurred at writing time." << std::endl;
        return 1;
    }

    std::vector<Employee> employee;
    Employee temp{};
    binaryFile.open(fileName, std::ios::binary | std::ios::in);
    while (!binaryFile.eof()) {
        binaryFile.read((char*)&temp, sizeof(Employee));
        employee.emplace_back(temp);
        temp = {};
    }
    std::cout << "server.exe: Printing \"" << fileName << "\": " << std::endl;
    for (auto &i : employee) {
        std::cout << i.num << " " << i.name << " " << i.hours << std::endl;
    }
    binaryFile.close();

    std::cout << "server.exe: Enter number of client processes: " << std::endl;
    std::cin >> procNum;
    if(procNum < 1) {
        std::cerr << "server.exe: Incorrect records number." << std::endl;
        return 1;
    }

    auto* si = new STARTUPINFO[procNum];
    auto* pi = new PROCESS_INFORMATION[procNum];

    auto* hPipe = new HANDLE[procNum];
    auto* hConnect = new BOOL[procNum];
    auto* hRead = new HANDLE[procNum];
    DWORD dwRead;
    for (int i = 0; i < procNum; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        std::string eventName = "hRead_" + std::to_string(i);
        hRead[i] = CreateEvent(nullptr, true, false, eventName.c_str());

        hPipe[i] = CreateNamedPipe("\\\\.\\pipe\\test_pipe",
                                   PIPE_ACCESS_DUPLEX,
                                   PIPE_TYPE_BYTE,// | PIPE_READMODE_BYTE | PIPE_WAIT,
                                   1,
                                   0,
                                   0,
                                   0,
                                   nullptr);
        if (hPipe[i] == nullptr || hPipe[i] == INVALID_HANDLE_VALUE) {
            std::cerr << "server.exe: Could not create pipe " << i << " (error " << GetLastError() << ")" << std::endl;
            return 1;
        }

        if (!CreateProcess(client.c_str(),
                           const_cast<char*>(fileName.c_str()),
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

        hConnect[i] = ConnectNamedPipe(hPipe[i], nullptr);
        if (!hConnect[i]) {
            std::cerr << "server.exe: Could not connect to pipe " << i << " (error " << GetLastError() << ")" << std::endl;
            CloseHandle(hPipe);
            return 1;
        }

    }

    //char buffer[1024];
    char* buffer = new char[1024];
    DWORD numBytesRead = 0;
    while(true) {
        BOOL result = ReadFile(
                hPipe[0],
                buffer, // the data from the pipe will be put here
                sizeof(buffer), // number of bytes allocated
                &numBytesRead, // this will store number of bytes actually read
                nullptr // not using overlapped IO
        );
        if (result) {
            buffer[numBytesRead] = '\0'; // null terminate the string
            std::cout << "server.exe: Number of bytes read: " << numBytesRead << std::endl;
            std::cout << "server.exe: Id: " << buffer << std::endl;
            break;
        }
    }

    //binaryFile.open(fileName, std::ios::binary | std::ios::in);
    std::string data;
    DWORD dwSent;
    for (auto &i : employee) {
        if (i.num == atoi(buffer)) {
            std::cout << "server.exe: Found record with id: " << i.num << " " << i.name << " " << i.hours << std::endl;
            data = std::to_string(i.num) + " " + i.name + " " + std::to_string(i.hours);
        }
    }

    BOOL hSend = WriteFile(
            hPipe[0],
            data.c_str(),
            data.size(),
            &dwSent,
            nullptr
    );
    if (hSend) {
        std::cout << "Number of bytes sent: " << dwSent << std::endl;
    } else {
        std::cerr << "Failed to send data. " << GetLastError() << std::endl;
    }

    //binaryFile.close();



    WaitForMultipleObjects(procNum, &pi->hProcess, true, INFINITE);

    //WaitForSingleObject(hRead[0], INFINITE);

    /*char buffer[1024];
    DWORD numBytesRead = 0;
    BOOL result = ReadFile(
            hPipe,
            buffer, // the data from the pipe will be put here
            sizeof(char), // number of bytes allocated
            &numBytesRead, // this will store number of bytes actually read
            nullptr // not using overlapped IO
    );
    if (result) {
        buffer[numBytesRead / sizeof(char)] = '\0'; // null terminate the string
        std::cout << "server.exe: Number of bytes read: " << numBytesRead << std::endl;
        std::cout << "server.exe: Message: " << buffer << std::endl;
    } else {
        std::cerr << "server.exe: Failed to read data from the pipe." << std::endl;
    }*/

    /*std::string data = "Hi client";
    hConnect[0] = WriteFile(
            hPipe[0], // handle to our outbound pipe
            data.c_str(), // data to send
            data.size(), // length of data to send (bytes)
            &dwRead, // will store actual amount of data sent
            nullptr // not using overlapped IO
    );
    if (hConnect[0]) {
        std::wcout << "Number of bytes sent: " << dwRead << std::endl;
    } else {
        std::wcout << "Failed to send data." << std::endl;
    }*/

    /*while (hPipe != INVALID_HANDLE_VALUE) {
        if (ConnectNamedPipe(hPipe, nullptr) != false) {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, nullptr) != false) {
                buffer[dwRead] = '\0';

                std::cout << "server.exe: Buffer: " << buffer << std::endl;
            }
        }

        DisconnectNamedPipe(hPipe);
    }*/


    /*std::wcout << "Creating an instance of a named pipe..." << std::endl;
    // Create a pipe to send data
    HANDLE pipe = CreateNamedPipe(
            "\\\\.\\pipe\\my_pipe", // name of the pipe
            PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
            PIPE_TYPE_BYTE, // send data as a byte stream
            1, // only allow 1 instance of this pipe
            0, // no outbound buffer
            0, // no inbound buffer
            0, // use default wait time
            NULL // use default security attributes
    );
    if (pipe == NULL || pipe == INVALID_HANDLE_VALUE) {
        std::wcout << "Failed to create outbound pipe instance.";
        // look up error code here using GetLastError()
        system("pause");
        return 1;
    }
    std::wcout << "Waiting for a client to connect to the pipe..." << std::endl;
    // This call blocks until a client process connects to the pipe
    BOOL result = ConnectNamedPipe(pipe, NULL);
    if (!result) {
        std::wcout << "Failed to make connection on named pipe." << std::endl;
        // look up error code here using GetLastError()
        CloseHandle(pipe); // close the pipe
        system("pause");
        return 1;
    }
    std::wcout << "Sending data to pipe..." << std::endl;
    // This call blocks until a client process reads all the data
    const wchar_t *data = L"*** Hello Pipe World ***";
    DWORD numBytesWritten = 0;
    result = WriteFile(
            pipe, // handle to our outbound pipe
            data, // data to send
            wcslen(data) * sizeof(wchar_t), // length of data to send (bytes)
            &numBytesWritten, // will store actual amount of data sent
            NULL // not using overlapped IO
    );
    if (result) {
        std::wcout << "Number of bytes sent: " << numBytesWritten << std::endl;
    } else {
        std::wcout << "Failed to send data." << std::endl;
        // look up error code here using GetLastError()
    }
    // Close the pipe (automatically disconnects client too)
    CloseHandle(pipe);
    std::wcout << "Done." << std::endl;
    system("pause");*/

    return 0;
}