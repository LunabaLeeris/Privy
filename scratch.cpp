#include <iostream>
#include <string.h>

#define CL "123456"

using namespace std;

int main() {
    int N = 12345;
    char buffer[7]; // Make sure buffer is large enough to hold the string representation of N


    // Use snprintf to format the integer into the buffer
    snprintf(buffer, 7, "%d", N);

    std::cout << "Buffer: " << buffer << std::endl;
    std::cout << strlen(buffer) << '\n';
    return 0;
}