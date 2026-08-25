#include "system.h"
#include "cpu.h"
#include "memory.h"
#include "gpu.h"
#include "battery.h"
#include <vector>
#include <iostream>
#include <sys/utsname.h>
#include <iomanip>

using namespace std;

void printSystemInfo(){
struct utsname systemInfo;


if(uname(&systemInfo)==0){
    MemoryInfo memory = getMemoryInfo();

    double totalGB = memory.totalKB / (1024.0 * 1024.0);
    double usedGB = memory.usedKB / (1024.0 * 1024.0);

    cout << "\033[1;36m";
    cout << "==========================================\n";
    cout << "        Linux System Information\n";
    cout << "==========================================\n\n";
    cout << "\033[0m";
    cout << "Operating System : " << getOSName() << endl;
    cout << "Hostname         : "
    << systemInfo.nodename << endl;
    cout << "Kernel Release   : "
    << systemInfo.release << endl;
    cout << "Kernel Version   : "
    << systemInfo.version << endl;
    cout << "Architecture     : "
    << systemInfo.machine << endl;
    cout << "CPU              : "
    << getCPUModel() << endl;
    cout << fixed << setprecision(2);

    cout << "Memory           : "
        << usedGB
        << " / "
        << totalGB
        << " GB"
        << endl;

    vector<GpuInfo> gpus = getGPUInfo();
    for (size_t i = 0; i < gpus.size(); ++i)
    {
        cout << (i == 0 ? "GPU              : " : "                   ")
             << gpus[i].name
             << " (" << gpus[i].type << ")"
             << endl;
    }

    BatteryInfo battery = getBatteryInfo();
    if (battery.present) {
        cout << "Battery          : "
             << battery.percent << "%";
        if (battery.status == "Charging")
            cout << " (Charging)";
        else if (battery.status == "Discharging")
            cout << " (Discharging)";
        else if (!battery.status.empty())
            cout << " (" << battery.status << ")";
        cout << endl;
    }
}
else
{
    cout << "Unable to retrieve system information.\n";
}
}
