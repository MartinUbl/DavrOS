#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "datasource.h"

struct DirectoryEntry
{
    char* name;
    int size_bytes;
};

class FileSystem
{
    public:
        virtual int Initialize(BlockDataSource*) = 0;
        virtual DirectoryEntry** GetDirectoryList() = 0;
        virtual char* ReadFile(const char* filename) = 0;

    protected:
        //

    private:
        //
};

#endif
