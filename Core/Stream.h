#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdint.h>
#include <SDL.h>
#include <stdio.h>
#include "Core.h"
#include <vector>

namespace emu
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
        void clear();
        virtual bool read(void* data, size_t size);
        virtual bool write(const void* data, size_t size);
        void* getBuffer();
        size_t getSize() const;

    private:
        std::vector<uint8_t>    mBuffer;
        size_t                  mReadPos;
        size_t                  mWritePos;
    };

    class CircularMemoryStream : public IStream
    {
    public:
        CircularMemoryStream(size_t size);
        virtual ~CircularMemoryStream();
        void clear();
        size_t getReadOffset() const;
        size_t getWritePos() const;
        bool setReadOffset(size_t offset);
        bool rewind(size_t offset);
        virtual bool read(void* data, size_t size);
        virtual bool write(const void* data, size_t size);

    private:
        std::vector<uint8_t>    mBuffer;
        bool                    mOverflow;
        size_t                  mAvailable;
        size_t                  mReadOffset;
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
