#include <string>
#include <regex>
#include <iostream>

#include "server.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "client.exe: Incorrect number of arguments." << std::endl;
        return 2;
    }
    std::string fileName = argv[0];
    std::string pipeName = argv[1];

    std::cout << "client.exe: Filename: " << fileName << std::endl;

    HANDLE hPipe;
    hPipe = CreateFile(pipeName.c_str(),
                       GENERIC_ALL,
                       0,
                       nullptr,
                       OPEN_EXISTING,
                       0,
                       nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cout << "client.exe: Could not open pipe (error " << GetLastError() << ")" << std::endl;
        return 1;
    }

    int key;
    int action = -1;
    DWORD dwSent = 0;
    DWORD dwRead = 0;
    Employee e{};

    while (action != 2) {
        std::cout << "client.exe: Print 0 to read, 1 to modify, 2 to exit: " << std::endl;
        std::cin >> action;
        if (action == 0) {
            std::cout << "client.exe: Read" << std::endl;
            std::cout << "client.exe: Enter employee id: " << std::endl;
            std::cin >> key;
            Request rRequest{0, key};

            if (!WriteFile(hPipe,&rRequest,sizeof(rRequest),&dwSent,nullptr)) {
                std::cerr << "client.exe: Failed to send data." << std::endl;
                return 1;
            }

            ReadFile(hPipe,&e,sizeof(e),&dwRead,nullptr);
            if (e.num == -1 && e.hours == -1) {
                std::cerr << "server.exe: Unable to find record with id " << key << std::endl;
            } else {
                std::cout << "client.exe: Read following data: " << e.num << " " << e.name << " " << e.hours << std::endl;
            }

            std::cout << "server.exe: Type '3' to release this record" << std::endl;
            std::cin >> action;
            while (action != 3) {
                std::cin >> action;
            }

        }
        else if (action == 1) {
            std::cout << "client.exe: Write" << std::endl;
            std::cout << "client.exe: Enter employee id: " << std::endl;
            std::cin >> key;
            Request wRequest{1, key};

            if (!WriteFile(hPipe,&wRequest,sizeof(wRequest),&dwSent,nullptr)) {
                std::cerr << "client.exe: Failed to send data." << std::endl;
            }

            ReadFile(hPipe,&e,sizeof(e),&dwRead,nullptr);
            if (e.num == -1 && e.hours == -1) {
                std::cerr << "server.exe: Unable to find record with id " << key << std::endl;
            } else {
                std::cout << "client.exe: Read following data: " << e.num << " " << e.name << " " << e.hours << std::endl;

                std::cout << "client.exe: Enter modified record: " << std::endl;
                std::cin >> e.name >> e.hours;
                if (!WriteFile(hPipe,&e,sizeof(e),&dwSent,nullptr)) {
                    std::cerr << "client.exe: Failed to send modified data." << std::endl;
                }

                std::cout << "client.exe: Sent modified data" << std::endl;
            }

        } else if (action == 2) {
            Request eRequest{2, 0};
            if (!WriteFile(hPipe,&eRequest,sizeof(eRequest),&dwSent,nullptr)) {
                std::cerr << "client.exe: Failed to send data." << std::endl;
                return 1;
            }
        }
    }


    CloseHandle(hPipe);
    return 0;
}