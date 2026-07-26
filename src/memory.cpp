#include "memory.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

MemoryInfo getMemoryInfo()
{
    MemoryInfo memory{};

    ifstream file("/proc/meminfo");

    if (!file)
    {
        cerr << "Unable to open /proc/meminfo\n";
        return memory;
    }

    string line;

    while (getline(file, line))
    {
        stringstream ss(line);

        string key;
        long long value;
        string unit;

        ss >> key >> value >> unit;

        if (key == "MemTotal:")
        {
            memory.totalKB = value;
        }
        else if (key == "MemAvailable:")
        {
            memory.availableKB = value;
        }

        if (memory.totalKB != 0 && memory.availableKB != 0)
        {
            break;
        }
    }

    memory.usedKB = memory.totalKB - memory.availableKB;

    return memory;
}