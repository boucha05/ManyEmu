#include "Stream.h"
#include <vector>
#include <cassert>

namespace NES
{
    MemoryStream::MemoryStream()
        : mReadPos(0)
        , mWritePos(0)
    {
        clear();
    }

    MemoryStream::~MemoryStream()
    {
    }

    void MemoryStream::clear()
    {
        mReadPos = 0;
        mWritePos = 0;
        mBuffer.clear();
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
            mBuffer.resize(mWritePos + size, 0);
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

    CircularMemoryStream::CircularMemoryStream(size_t size)
    {
        mBuffer.resize(size, 0);
        clear();
    }

    CircularMemoryStream::~CircularMemoryStream()
    {
    }

    void CircularMemoryStream::clear()
    {
        mOverflow = 0;
        mAvailable = 0;
        mReadOffset = 0;
        mWritePos = 0;
    }

    size_t CircularMemoryStream::getReadOffset() const
    {
        return mReadOffset;
    }

    size_t CircularMemoryStream::getWritePos() const
    {
        return mWritePos;
    }

    bool CircularMemoryStream::setReadOffset(size_t offset)
    {
        if (offset > mAvailable)
        {
            mOverflow = true;
            return false;
        }

        mOverflow = false;
        mReadOffset = offset;
        return true;
    }

    bool CircularMemoryStream::rewind(size_t offset)
    {
        if (offset > mAvailable)
        {
            mAvailable = 0;
            mOverflow = true;
            return false;
        }

        mAvailable -= offset;
        if (mReadOffset < offset)
            mOverflow = true;
        mReadOffset -= offset;
        if (mWritePos < offset)
            mWritePos += mBuffer.size();
        mWritePos -= offset;
        return true;
    }

    bool CircularMemoryStream::read(void* data, size_t size)
    {
        if (mOverflow || (size > mReadOffset))
        {
            memset(data, 0, size);
            return false;
        }

        uint8_t* dst = static_cast<uint8_t*>(data);
        size_t bufferSize = mBuffer.size();
        while (size)
        {
            size_t readPos = mWritePos - mReadOffset;
            if (mWritePos < mReadOffset)
                readPos += bufferSize;

            size_t copySize = bufferSize - readPos;
            if (copySize > size)
                copySize = size;
            memcpy(dst, &mBuffer[readPos], copySize);
            size -= copySize;

            dst += copySize;

            mReadOffset -= copySize;
            assert((uint32_t)mReadOffset < bufferSize);
        }
        return true;
    }

    bool CircularMemoryStream::write(const void* data, size_t size)
    {
        const uint8_t* src = static_cast<const uint8_t*>(data);
        size_t bufferSize = mBuffer.size();
        while (size)
        {
            size_t copySize = bufferSize - mWritePos;
            if (copySize > size)
                copySize = size;
            memcpy(&mBuffer[mWritePos], src, copySize);
            size -= copySize;
            
            src += copySize;
            
            mWritePos += copySize;
            if (mWritePos >= bufferSize)
                mWritePos -= bufferSize;

            mAvailable += copySize;
            if (mAvailable >= bufferSize)
                mAvailable = bufferSize;

            mReadOffset += copySize;
            if (mReadOffset >= bufferSize)
            {
                mReadOffset = bufferSize - 1;
                mOverflow = true;
            }
        }
        return true;
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