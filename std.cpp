#include "std.h"
#include "framebuffer.h"

void memset(void* address, char value, int length)
{
    int i;
    // fill address range with supplied character
    for (i = 0; i < length; i++)
        ((char*)address)[i] = value;
}

int strlen(const char* c)
{
    int i = 0;
    // advance 'i' until we reach zero
    while (c[i++] != '\0')
        ;

    return i;
}

int strcmp(const char* first, const char* second)
{
    int i;

    // go from the beginning to the end of at least one string
    for (i = 0; first[i] != '\0' && second[i] != '\0'; i++)
    {
        // if strings does not match
        if (first[i] != second[i])
        {
            if (first[i] > second[i])
                return 1;
            return -1;
        }
    }

    return 0;
}

int strncmp(const char* first, const char* second, int maxlength)
{
    int i;

    // go from the beginning to the end of at least one string
    for (i = 0; first[i] != '\0' && second[i] != '\0' && i < maxlength; i++)
    {
        // if strings does not match
        if (first[i] != second[i])
        {
            if (first[i] > second[i])
                return 1;
            return -1;
        }
    }

    return 0;
}

int atoi(char* buffer)
{
    int tmp = 0;

    while (*buffer != '\0')
    {
        tmp *= 10;
        tmp += (*buffer) - '0';
        buffer++;
    }

    return tmp;
}

void itoa(int src, char* dstbuffer)
{
    int i, j;
    char c;

    for (i = 0; src > 0; i++)
    {
        dstbuffer[i] = '0' + src % 10;
        src = src / 10;
    }

    i--;

    for (j = 0; j <= i/2; j++)
    {
        c = dstbuffer[j];
        dstbuffer[j] = dstbuffer[i-j];
        dstbuffer[i-j] = c;
    }

    dstbuffer[i+1] = '\0';
}
