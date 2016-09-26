#include <sys/stat.h>
#include <stdio.h>

long long int fileSize(const char *filename) 
{
    struct stat st; 

    if (stat(filename, &st) == 0)
    {        
        return st.st_size;
    }

    return -1; 
}
