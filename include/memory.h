#ifndef MEMORY_H
#define MEMORY_H

struct MemoryInfo{
        long long totalKB;
        long long availableKB;
        long long usedKB;

};
MemoryInfo getMemoryInfo();
#endif
