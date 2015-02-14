#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdint.h>
#include <SDL.h>
#include <stdio.h>
#include "nes.h"
#include <vector>

namespace NES
{
    class IStream
    {
    public:
        virtual bool read(void* data, size_t size) = 0;
        virtual bool write(const void* data, size_t size) = 0;
    };

    class MemoryStream : public IStream
    {
    public:
        MemoryStream();
        virtual ~MemoryStream();
        virtual bool read(void* data, size_t size);
        virtual bool write(const void* data, size_t size);
        void* getBuffer();
        size_t getSize() const;

    private:
        std::vector<uint8_t>    mBuffer;
        size_t                  mReadPos;
        size_t                  mWritePos;
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
