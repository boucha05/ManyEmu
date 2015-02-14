#include "Stream.h"
#include <vector>

namespace NES
{
    MemoryStream::MemoryStream()
        : mReadPos(0)
        , mWritePos(0)
    {
    }

    MemoryStream::~MemoryStream()
    {
    }

    bool MemoryStream::read(void* data, size_t size)
    {
        bool success = mReadPos + size <= mBuffer.size();
        if (!success)
            size = mBuffer.size() - mReadPos;
        memcpy(data, &mBuffer[mReadPos], size);
        mReadPos += size;
        return success;
    }

    bool MemoryStream::write(const void* data, size_t size)
    {
        if ((mWritePos + size) > mBuffer.size())
        {
            mBuffer.resize(size, 0);
            memcpy(&mBuffer[mWritePos], data, size);
        }
        mWritePos += size;
        return true;
    }

    void* MemoryStream::getBuffer()
    {
        return (mBuffer.size() > 0) ? &mBuffer[0] : nullptr;
    }

    size_t MemoryStream::getSize() const
    {
        return mBuffer.size();
    }

    ///////////////////////////////////////////////////////////////////////////

    FileStream::FileStream(const char* path, const char* mode)
        : mFile(nullptr)
    {
        mFile = fopen(path, mode);
    }

    FileStream::~FileStream()
    {
        close();
    }

    bool FileStream::valid()
    {
        return mFile != NULL;
    }

    bool FileStream::read(void* data, size_t size)
    {
        return mFile && fread(data, 1, size, mFile) > 0;
    }

    bool FileStream::write(const void* data, size_t size)
    {
        return mFile && fwrite(data, 1, size, mFile) > 0;
    }

    void FileStream::close()
    {
        if (mFile)
        {
            fclose(mFile);
            mFile = nullptr;
        }
    }
}