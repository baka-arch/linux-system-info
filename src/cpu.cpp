#include <iostream>
#include <fstream>

using namespace std;
string getCPUModel()
{
    ifstream file("/proc/cpuinfo");

    if (!file)
    {
        cerr << "Error: Unable to open /proc/cpuinfo\n";
        return "Unknown";
    }

    string line;

    while (getline(file, line))
    {
        if (line.find("model name") != string::npos)
        {
            size_t pos = line.find(":");
            return line.substr(pos + 2);
        }
    }

    return "Unknown";
}