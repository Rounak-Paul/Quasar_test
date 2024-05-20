#pragma once

#include <qspch.h>

namespace Quasar
{
class Filesystem {
public:
    void Write(const std::string& filename, const std::string& data);

    void Append(const std::string& filename, const std::string& data);

    std::string Read(const std::string& filename);

    std::vector<char> ReadBinary(const std::string& filename);

    bool Delete(const std::string& filename);

    bool Exists(const std::string& filename);
};

} // namespace Quasar
