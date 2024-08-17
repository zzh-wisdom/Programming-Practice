#include <iostream>

int main() {
    unsigned int size = 16;
    unsigned int mask = size - 1;
    unsigned int in = 6;
    unsigned int out = ~0u - 1; // 0xfffffffe，相当于14，尾部有2字节空间

    unsigned int used = in - out;
    unsigned int unused = size - in + out;

    printf("sizeof(unsigned int): %lu\n", sizeof(unsigned int));
    std::cout << "used: " << used << " unused:" << unused << std::endl;
    return 0;
}