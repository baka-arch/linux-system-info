#include "gpu.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <cstdio>
#include <map>
#include <cstring>

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

// Clean an lspci-style device string down to just the GPU model name,
// e.g. "NVIDIA Corporation AD107M [GeForce RTX 4050 Max-Q / Mobile]"
//   -> "GeForce RTX 4050"
//    "Advanced Micro Devices, Inc. [AMD/ATI] Rembrandt [Radeon 680M]"
//   -> "Radeon 680M"
string cleanGpuName(string name)
{
    // Prefer the text inside the square brackets (the marketing name),
    // skipping the "[AMD/ATI]"-style vendor tag.
    size_t best = string::npos;
    size_t pos = 0;
    while ((pos = name.find('[', pos)) != string::npos)
    {
        size_t close = name.find(']', pos);
        if (close == string::npos)
            break;
        string candidate = name.substr(pos + 1, close - pos - 1);
        // Skip short vendor tags like "AMD/ATI" or "VGA controller".
        if (candidate.size() <= 12 && candidate.find('/') != string::npos)
        {
            pos = close + 1;
            continue;
        }
        best = pos;
        break;
        pos = close + 1;
    }

    if (best != string::npos)
    {
        size_t start = name.find('[', best);
        size_t end = name.find(']', start);
        name = name.substr(start + 1, end - start - 1);
    }
    else
    {
        // No useful brackets: drop common vendor prefixes.
        const char *prefixes[] = {
            "NVIDIA Corporation ",
            "Advanced Micro Devices, Inc. ",
            "Intel Corporation ",
        };
        for (const char *p : prefixes)
        {
            size_t plen = strlen(p);
            if (name.compare(0, plen, p) == 0)
            {
                name = name.substr(plen);
                break;
            }
        }
    }

    // Drop variant suffixes like "Max-Q / Mobile", "Mobile", "M".
    const char *suffixes[] = {" Max-Q / Mobile", " Max-Q", " Mobile", " M"};
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const char *s : suffixes)
        {
            size_t slen = strlen(s);
            if (name.size() > slen &&
                name.compare(name.size() - slen, slen, s) == 0)
            {
                name.resize(name.size() - slen);
                changed = true;
            }
        }
    }

    return trim(name);
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
        string raw = resolveNameFromPciIds(vendor, readSysfs(base + "/device"));
        info.name = cleanGpuName(raw);
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
                string raw = trim(line.substr(pos + 2));

                // Strip the trailing kernel-driver annotation lspci adds,
                // e.g. "(rev a1)".
                size_t rev = raw.find("(rev ");
                if (rev != string::npos)
                    raw = trim(raw.substr(0, rev));

                info.type = (raw.find("NVIDIA") != string::npos)
                                ? "Discrete"
                                : "Integrated";
                info.name = cleanGpuName(raw);
                gpus.push_back(info);
            }
        }
    }

    if (!gpus.empty())
        return gpus;

    // Fallback: enumerate drm cards from sysfs directly.
    return detectGPUsFromSysfs();
}
