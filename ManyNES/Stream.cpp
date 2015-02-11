#include "Stream.h"
#include <vector>

namespace NES
{
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