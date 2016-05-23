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

void str_reverse(char* str, int length)
{
    int start = 0;
    int end = length -1;
    char tmp;
    while (start < end)
    {
        tmp = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = tmp;
        start++;
        end--;
    }
}

void itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
    int rem;

    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
    }

    // consider every other base number as unsigned
    if (num < 0 && base == 10)
    {
        isNegative = true;
        num = -num;
    }

    while (num != 0)
    {
        rem = num % base;
        str[i++] = (rem > 9) ? (rem-10) + 'A' : rem + '0';
        num = num/base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    str_reverse(str, i);
}
