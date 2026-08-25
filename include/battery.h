#ifndef BATTERY_H
#define BATTERY_H

#include <string>

struct BatteryInfo {
    bool present = false;
    int percent = 0;
    std::string status;   // e.g. "Charging", "Discharging", "Full"
};

BatteryInfo getBatteryInfo();

#endif
