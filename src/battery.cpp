#include "battery.h"
#include <fstream>
#include <dirent.h>
#include <string>

using namespace std;

static string readSysFile(const string &path) {
    ifstream file(path);
    string value;
    if (getline(file, value))
        return value;
    return "";
}

BatteryInfo getBatteryInfo() {
    BatteryInfo info;

    DIR *dir = opendir("/sys/class/power_supply");
    if (!dir) return info;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name.rfind("BAT", 0) != 0) continue;

        string base = "/sys/class/power_supply/" + name;
        string status = readSysFile(base + "/status");
        string capacity = readSysFile(base + "/capacity");

        if (status.empty() || capacity.empty()) continue;

        info.present = true;
        info.status = status;
        try {
            info.percent = stoi(capacity);
        } catch (...) {
            info.percent = 0;
        }
        break; // first valid battery is enough
    }
    closedir(dir);
    return info;
}
