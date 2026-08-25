#include "gpu.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <cstdio>
#include <map>

using namespace std;

namespace
{

// Known PCI vendor IDs for classifying GPUs as integrated vs discrete.
const string AMD_VENDOR = "0x1002";
const string NVIDIA_VENDOR = "0x10de";
const string INTEL_VENDOR = "0x8086";

bool isDiscrete(const string &vendor)
{
    // NVIDIA only makes discrete GPUs.
    return vendor == NVIDIA_VENDOR;
}

string trim(const string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Read the first N bytes of a small sysfs file into a trimmed string.
string readSysfs(const string &path)
{
    ifstream file(path);
    string value;
    if (file)
    {
        getline(file, value);
    }
    return trim(value);
}

// Resolve a PCI ID pair (e.g. 0x1002:0x1681) to a human-readable GPU name.
string resolveNameFromPciIds(const string &vendor, const string &device)
{
    static const map<string, map<string, string>> knownDevices = {
        {INTEL_VENDOR, {{"0x46a6", "Intel Iris Xe Graphics"}}},
        {NVIDIA_VENDOR, {}}, // names vary too much; show generic label below
        {AMD_VENDOR, {}}};

    auto vit = knownDevices.find(vendor);
    if (vit != knownDevices.end())
    {
        auto dit = vit->second.find(device);
        if (dit != vit->second.end())
        {
            return dit->second;
        }
    }

    if (vendor == NVIDIA_VENDOR)
        return "NVIDIA Discrete GPU (" + device + ")";
    if (vendor == AMD_VENDOR)
        return "AMD Integrated Graphics (" + device + ")";
    if (vendor == INTEL_VENDOR)
        return "Intel Integrated Graphics (" + device + ")";
    return "Unknown GPU (" + vendor + ":" + device + ")";
}

vector<GpuInfo> detectGPUsFromSysfs()
{
    vector<GpuInfo> gpus;

    for (int card = 0; card < 32; ++card)
    {
        string base = "/sys/class/drm/card" + to_string(card) + "/device";
        string vendor = readSysfs(base + "/vendor");

        if (vendor.empty())
            continue;

        GpuInfo info;
        info.name = resolveNameFromPciIds(vendor, readSysfs(base + "/device"));
        info.type = isDiscrete(vendor) ? "Discrete" : "Integrated";
        gpus.push_back(info);
    }

    return gpus;
}

} // namespace

vector<GpuInfo> getGPUInfo()
{
    // Preferred path: ask lspci for all display/VGA/3D controllers.
    array<char, 256> buffer;
    vector<GpuInfo> gpus;

    FILE *pipe = popen("lspci 2>/dev/null | grep -E 'VGA|3D|Display'", "r");
    if (pipe)
    {
        string output;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            output += buffer.data();
        }
        pclose(pipe);

        if (!output.empty())
        {
            stringstream ss(output);
            string line;
            while (getline(ss, line))
            {
                size_t pos = line.find(": ");
                if (pos == string::npos)
                    continue;

                GpuInfo info;
                info.name = trim(line.substr(pos + 2));

                // Strip the trailing kernel-driver annotation lspci adds,
                // e.g. "(rev a1)" or "(prog-if 00 [VGA controller])".
                size_t rev = info.name.find("(rev ");
                if (rev != string::npos)
                    info.name = trim(info.name.substr(0, rev));

                info.type = (info.name.find("NVIDIA") != string::npos)
                                ? "Discrete"
                                : "Integrated";
                gpus.push_back(info);
            }
        }
    }

    if (!gpus.empty())
        return gpus;

    // Fallback: enumerate drm cards from sysfs directly.
    return detectGPUsFromSysfs();
}
