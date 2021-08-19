#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pasta {

std::optional<std::string> readFileStr(const std::filesystem::path& path);
std::optional<std::vector<uint8_t>> readFileBin(const std::filesystem::path& path);
std::filesystem::path exeDirectory(const std::string& argv0);

}
