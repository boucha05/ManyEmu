#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdint.h>
#include <SDL.h>
#include <stdio.h>
#include "nes.h"

namespace NES
{
    class IStream
    {
    public:
        virtual bool read(void* data, size_t size) = 0;
        virtual bool write(const void* data, size_t size) = 0;
    };

    class FileStream : public IStream
    {
    public:
        FileStream(const char* path, const char* mode);
        virtual ~FileStream();
        virtual bool valid();
        virtual bool read(void* data, size_t size);
        virtual bool write(const void* data, size_t size);
        void close();

    private:
        FileStream();

        FILE*   mFile;
    };
}

#endif
