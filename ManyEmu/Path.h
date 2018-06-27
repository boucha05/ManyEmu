#pragma once
#ifndef __PATH_H__
#define __PATH_H__

#include <vector>
#include <string>

class Path
{
public:
    typedef std::vector<std::string> FileList;

    static std::string join(const std::string& path1, const std::string& path2);
    static void makeDirs(const std::string& path);
    static void removeFile(const std::string& path);
    static std::string normalizePath(const std::string& path);
    static std::string normalizeCase(const std::string& path);
    static void split(const std::string& path, std::string& head, std::string& tail);
    static void splitExt(const std::string& path, std::string& root, std::string& ext);
    static bool walkFiles(FileList& fileList, const std::string& directory, const std::string& prefix, const std::string& filter, bool recursive);

private:
    Path();
    ~Path();
};

#endif