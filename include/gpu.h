#ifndef GPU_H
#define GPU_H

#include <string>
#include <vector>

struct GpuInfo
{
    std::string name;
    std::string type; // "Integrated" or "Discrete"
};

std::vector<GpuInfo> getGPUInfo();

#endif
