#include <iostream>

using namespace std;


inline unsigned long long rdtsc1()
{
    unsigned int high, low;
    asm("rdtsc          \n\t"
        "mov %%eax, %0 \n\t"
        "mov %%edx, %1     ": "=m"(low), "=m"(high));

    return (((unsigned long long int)high) << 32) | (low);
}

inline unsigned long long rdtsc2()
{
#if defined(__i386__)

    unsigned long long int tick;
    __asm__ __volatile__("rdtsc" : "=A"(tick));
    return tick;

#elif defined(__x86_64__)

    unsigned int high, low;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((unsigned long long int)high << 32) | low;

#else
#   error
#endif
}

int main()
{
    long long int r;


    r =  __builtin_ia32_rdtsc();

    cout << "__builtin_ia32_rdtsc: " << r << endl;


    r = rdtsc1();

    cout << "rdtsc1: " << r << endl;


    r = rdtsc2();

    cout << "rdtsc2: " << r << endl;

    return 0;
}

