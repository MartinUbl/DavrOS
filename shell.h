#ifndef __SHELL_H__
#define __SHELL_H__

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

class DefaultShell : public ShellInterface
{
    public:
        DefaultShell();
        void Run();

    protected:
        //

    private:
        //
};

#endif
