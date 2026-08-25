#include "system.h"
#include <fstream>
#include <string>

using namespace std;

// Reads the pretty OS name (e.g. "Arch Linux") from /etc/os-release
std::string getOSName() {
    ifstream file("/etc/os-release");
    string line;
    while (getline(file, line)) {
        if (line.rfind("PRETTY_NAME=", 0) == 0) {
            string name = line.substr(12);
            if (!name.empty() && name.front() == '"') name = name.substr(1);
            if (!name.empty() && name.back() == '"') name.pop_back();
            return name;
        }
    }
    return "Unknown";
}
