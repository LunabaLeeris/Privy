#include <winsock2.h>
#include <iostream>

#define ADDR "127.0.0.7"
#define PORT 1567
#define BUFFERSIZE 1056

void test(int to_check, std::string message){
    if (to_check < 0){
        std::cout << "FAILED: " << message << '\n';
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "SUCCESS: " << message << '\n';
    }
}

void place(fd_set *from, fd_set *to){
    for (int i = 0; i < from->fd_count; i++){
        FD_SET(from->fd_array[i], to);
    }
}