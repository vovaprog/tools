#include <bitset>

#include <iostream>
#include <iomanip>

static void printHex(void *buf, int size)
{
    std::cout << std::hex;
    std::cout << std::endl << "[ ";
    for(int i = 0; i < size; ++i)
    {
        std::cout << std::setfill('0') << std::setw(2) << (int)((unsigned char*)buf)[i] << " ";
    }
    std::cout << "]   (size: " << std::dec << size << ")" << std::endl;
}

static void printHex(const char *msg, void *buf, int size)
{
    std::cout << msg;
    printHex(buf, size);
}

static void printBin(void *buf, int size)
{
    std::cout << std::endl << "[ ";

    for(int i = 0; i < size; ++i)
    {
        std::cout << std::bitset<8>(static_cast<unsigned int>((static_cast<unsigned char*>(buf))[i])) << " ";
    }

    std::cout << "]   (size: " << std::dec << size << ")" << std::endl;
}

static void printBin(const char *msg, void *buf, int size)
{
    std::cout << msg;
    printBin(buf, size);
}


int main()
{
    char buf[] = {0x11, 0x22, 0x33};
    int size = 3;

    printHex(buf, size);
    printHex("input: ", buf, size);
    printHex("output: ", buf, size);
    printBin(buf, size);
    printBin("input: ", buf, size);
    printBin("output: ", buf, size);

    return 0;
}

