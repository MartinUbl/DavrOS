#ifndef __DATASOURCE_H__
#define __DATASOURCE_H__

class BlockDataSource
{
    public:
        // jumps to offset in data source (does not seek now)
        virtual int SeekTo(int offset) = 0;
        // retrieves current offset
        virtual int GetPosition() = 0;
        // reads bytes from current location
        virtual int ReadBytes(char* target, int count) = 0;
        // writes bytes to current location
        virtual int WriteBytes(char* source, int count) = 0;

    protected:
        //

    private:
        //
};

#endif
