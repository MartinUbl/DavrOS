#ifndef __SHELL_H__
#define __SHELL_H__

#include "stdint.h"
#include "filesystem.h"

class ShellInterface
{
    public:
        ShellInterface() { };

        virtual void Run() { };

    protected:
        //

    private:
        //
};

enum ShellLocationType
{
    SLT_NONE = 0,
    SLT_FLOPPY = 1,

    MAX_SLT
};

class DefaultShell : public ShellInterface
{
    public:
        DefaultShell();
        void Run();

    protected:
        uint32_t m_locationType;
        FileSystem* m_currentFS;

    private:
        //
};

#endif
