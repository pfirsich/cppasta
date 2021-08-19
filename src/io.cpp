#include "cppasta/io.hpp"

#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace pasta {

// Reading a fucking file in this insane language is so incredibly hard to do right:
// https://web.archive.org/web/20180314195042/http://cpp.indi.frih.net/blog/2014/09/how-to-read-an-entire-file-into-memory-in-cpp/
// https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

std::optional<std::string> readFileStr(const fs::path& path)
{
    std::ifstream file(path);
    if (!file)
        return std::nullopt;

    std::ostringstream ss;
    if (!(ss << file.rdbuf()))
        return std::nullopt;
    return ss.str();
}

std::optional<std::vector<uint8_t>> readFileBin(const fs::path& path)
{
    std::ifstream file(path, std::ios_base::binary);
    if (!file)
        return std::nullopt;
    std::ostringstream ss;
    if (!(ss << file.rdbuf()))
        return std::nullopt;

    // Copying twice here is shit, but I don't know how to get rid of it
    const auto str = ss.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

fs::path exeDirectory(const std::string& argv0)
{
    return fs::absolute(argv0).parent_path();
}

}
