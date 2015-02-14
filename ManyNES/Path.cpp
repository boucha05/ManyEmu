#include "Path.h"
#include <algorithm>
#include <direct.h>
#include <io.h>

std::string Path::join(const std::string& path1, const std::string& path2)
{
    // TODO: Reject path1 if path2 is absolute
    std::string path = path1;
    if (path.empty() || path.back() != '\\')
        path += "\\";
    path += path2;
    return path;
}

namespace
{
    void makeDirsImpl(const std::string& normalizedPath)
    {
        std::string base, directory;
        Path::split(normalizedPath, base, directory);
        if(!base.empty())
            makeDirsImpl(base);
        if(!directory.empty() && directory != ".." && directory != ".")
            _mkdir(normalizedPath.c_str());
    }
}

void Path::makeDirs(const std::string& path)
{
    makeDirsImpl(normalizePath(path));
}

void Path::removeFile(const std::string& path)
{
    remove(path.c_str());
}

std::string Path::normalizePath(const std::string& path)
{
    std::string result = path;

    // Convert '/' to '\'
    size_t pos = result.find_first_of("/");
    while(pos != std::string::npos)
    {
        result[pos] = '\\';
        pos = result.find_first_of("/", pos);
    }

    // TODO: Collapse 'XXX\..'

    // Convert '.\' to '\'
    pos = result.find(".\\");
    while(pos != std::string::npos)
    {
        result = result.substr(0, pos) + result.substr(pos + 1);
        pos = result.find(".\\", pos);
    }

    // Convert '\\' to '\'
    pos = result.find("\\\\");
    while(pos != std::string::npos)
    {
        result = result.substr(0, pos) + result.substr(pos + 1);
        pos = result.find("\\\\", pos);
    }

    return result;
}

std::string Path::normalizeCase(const std::string& path)
{
    std::string result = path;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void Path::split(const std::string& path, std::string& head, std::string& tail)
{
    size_t separator = path.find_last_of("\\/");
    if(separator != std::string::npos)
    {
        head = path.substr(0, separator);
        if(separator + 1 < path.size())
            tail = path.substr(separator + 1);
    }
    else
    {
        head = "";
        tail = path;
    }
}

void Path::splitExt(const std::string& path, std::string& root, std::string& ext)
{
    size_t separator = path.find_last_of("\\/.");
    if((separator != std::string::npos) && path[separator] == '.')
    {
        root = path.substr(0, separator);
        if(separator + 1 < path.size())
            ext = path.substr(separator + 1);
    }
    else
    {
        root = path;
    }
}

bool Path::walkFiles(FileList& fileList, const std::string& directory, const std::string& prefix, const std::string& filter, bool recursive)
{
    std::string pattern = Path::join(directory, filter);
    _finddata_t data;
    intptr_t iter = _findfirst(pattern.c_str(), &data);
    bool success = iter != -1;
    bool more = success;
    while(more)
    {
        std::string name = data.name;
        std::string filename = prefix + name;

        // Check for a valid file
        if(data.attrib & _A_SUBDIR)
        {
            // This is a directoy, filter directory
            if((name != ".") && (name != "..") && recursive)
            {
                // Valid directory, proceed with recursion
                std::string subdirectory = Path::join(directory, name);
                walkFiles(fileList, subdirectory, filename + "\\", filter, recursive);
            }
        }
        else
        {
            // Valid file, add to the path
            fileList.push_back(filename.c_str());
        }

        // Get next entry
        more = _findnext(iter, &data) == 0;
    }
    _findclose(iter);
    return success;
}
