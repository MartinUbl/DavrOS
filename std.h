#ifndef __STD_H__
#define __STD_H__

// fills address range with supplied character
void memset(void* address, char value, int length);

// measures length of zero-terminated string
int strlen(const char* c);
// compares two strings, returns 0 if equal, 1 or -1 if not
int strcmp(const char* first, const char* second);
// compares two strings, returns 0 if equal, 1 or -1 if not; limits range by maxlength
int strncmp(const char* first, const char* second, int maxlength);

#endif