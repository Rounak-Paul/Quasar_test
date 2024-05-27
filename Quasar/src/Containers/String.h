#pragma once
#include <string>

namespace Quasar {
class String : public std::string {
public:
    using std::string::string;

    String(const std::string& str) : std::string(str) {}

    String& operator=(const std::string& str) {
        // string
        std::string::operator=(str); // Call base class method
        return *this;
    }

};
}
