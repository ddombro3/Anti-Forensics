#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <thread>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "third_party/vmaware.hpp"

#ifdef _WIN32
    #include <windows.h>
    #include <intrin.h>
#else
    //#include <cpuid.h>
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <sys/utsname.h>
#endif

struct MemoryInfo {
    std::uint64_t total_bytes = 0;
    std::uint64_t available_bytes = 0;
};

struct DiskInfo {
    std::uint64_t capacity_bytes = 0;
    std::uint64_t free_bytes = 0;
    std::uint64_t available_bytes = 0;
};

struct SystemTelemetry {
    std::string os_name;
    std::string hostname;
    std::string architecture;
    std::string cpu_brand;
    unsigned int logical_cores = 0;
    std::uint64_t page_size = 0;
    std::uint64_t uptime_seconds = 0;
    MemoryInfo memory;
    DiskInfo disk;
};

std::string format_bytes(std::uint64_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unit_index = 0;

    while (value >= 1024.0 && unit_index < 4) {
        value /= 1024.0;
        ++unit_index;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value << ' ' << units[unit_index];
    return out.str();
}

std::string get_os_name() {
#ifdef _WIN32
    return "Windows";
#else
    return "Linux";
#endif
}

std::string get_hostname() {
#ifdef _WIN32
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return std::string(buffer, size);
    }
    return "Unknown";
#else
    char buffer[256] = {};
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
    }
    return "Unknown";
#endif
}

std::string get_architecture() {
#ifdef _WIN32
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);

    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        case PROCESSOR_ARCHITECTURE_ARM:   return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
        default: return "Unknown";
    }
#else
    struct utsname info{};
    if (uname(&info) == 0) {
        return info.machine;
    }
    return "Unknown";
#endif
}

std::string get_cpu_brand() {
    char brand[49] = {};
#ifdef _WIN32
    int cpu_info[4] = {};

    __cpuid(cpu_info, 0x80000000);
    unsigned int max_extended = static_cast<unsigned int>(cpu_info[0]);
    if (max_extended < 0x80000004) {
        return "Unknown";
    }

    __cpuid(cpu_info, 0x80000002);
    std::memcpy(brand + 0, cpu_info, 16);

    __cpuid(cpu_info, 0x80000003);
    std::memcpy(brand + 16, cpu_info, 16);

    __cpuid(cpu_info, 0x80000004);
    std::memcpy(brand + 32, cpu_info, 16);

    return std::string(brand);
#else
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    unsigned int max_extended = __get_cpuid_max(0x80000000, nullptr);

    if (max_extended < 0x80000004) {
        return "Unknown";
    }

    __get_cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
    std::memcpy(brand + 0, &eax, 4);
    std::memcpy(brand + 4, &ebx, 4);
    std::memcpy(brand + 8, &ecx, 4);
    std::memcpy(brand + 12, &edx, 4);

    __get_cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
    std::memcpy(brand + 16, &eax, 4);
    std::memcpy(brand + 20, &ebx, 4);
    std::memcpy(brand + 24, &ecx, 4);
    std::memcpy(brand + 28, &edx, 4);

    __get_cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
    std::memcpy(brand + 32, &eax, 4);
    std::memcpy(brand + 36, &ebx, 4);
    std::memcpy(brand + 40, &ecx, 4);
    std::memcpy(brand + 44, &edx, 4);

    return std::string(brand);
#endif
}

unsigned int get_logical_cores() {
    unsigned int cores = std::thread::hardware_concurrency();
    return cores == 0 ? 1 : cores;
}

std::uint64_t get_page_size() {
#ifdef _WIN32
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    return static_cast<std::uint64_t>(si.dwPageSize);
#else
    long page_size = sysconf(_SC_PAGESIZE);
    return page_size > 0 ? static_cast<std::uint64_t>(page_size) : 0;
#endif
}

MemoryInfo get_memory_info() {
    MemoryInfo info{};

#ifdef _WIN32
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);

    if (GlobalMemoryStatusEx(&mem)) {
        info.total_bytes = mem.ullTotalPhys;
        info.available_bytes = mem.ullAvailPhys;
    }
#else
    struct sysinfo mem{};
    if (sysinfo(&mem) == 0) {
        info.total_bytes = static_cast<std::uint64_t>(mem.totalram) * mem.mem_unit;
        info.available_bytes = static_cast<std::uint64_t>(mem.freeram) * mem.mem_unit;
    }
#endif

    return info;
}

std::uint64_t get_uptime_seconds() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(GetTickCount64() / 1000ULL);
#else
    struct sysinfo s{};
    if (sysinfo(&s) == 0) {
        return static_cast<std::uint64_t>(s.uptime);
    }
    return 0;
#endif
}

DiskInfo get_disk_info(const std::filesystem::path& path = ".") {
    DiskInfo info{};

    try {
        auto space = std::filesystem::space(path);
        info.capacity_bytes = space.capacity;
        info.free_bytes = space.free;
        info.available_bytes = space.available;
    } catch (...) {
        // Leave zeros if query fails
    }

    return info;
}

SystemTelemetry collect_telemetry() {
    SystemTelemetry t{};

    t.os_name = get_os_name();
    t.hostname = get_hostname();
    t.architecture = get_architecture();
    t.cpu_brand = get_cpu_brand();
    t.logical_cores = get_logical_cores();
    t.page_size = get_page_size();
    t.uptime_seconds = get_uptime_seconds();
    t.memory = get_memory_info();
    t.disk = get_disk_info(".");

    return t;
}

void print_telemetry(const SystemTelemetry& t) {
    std::cout << "=== System Telemetry ===\n";
    std::cout << "OS:                " << t.os_name << '\n';
    std::cout << "Hostname:          " << t.hostname << '\n';
    std::cout << "Architecture:      " << t.architecture << '\n';
    std::cout << "CPU:               " << t.cpu_brand << '\n';
    std::cout << "Logical Cores:     " << t.logical_cores << '\n';
    std::cout << "Page Size:         " << t.page_size << " bytes\n";
    std::cout << "Uptime:            " << t.uptime_seconds << " seconds\n";
    std::cout << "Total RAM:         " << format_bytes(t.memory.total_bytes) << '\n';
    std::cout << "Available RAM:     " << format_bytes(t.memory.available_bytes) << '\n';
    std::cout << "Disk Capacity:     " << format_bytes(t.disk.capacity_bytes) << '\n';
    std::cout << "Disk Free:         " << format_bytes(t.disk.free_bytes) << '\n';
    std::cout << "Disk Available:    " << format_bytes(t.disk.available_bytes) << '\n';
}




int main() {
    SystemTelemetry telemetry = collect_telemetry();
    print_telemetry(telemetry);

        const std::uint8_t percent = VM::percentage();

    if (percent == 100) {
        std::cout << "Definitely a VM!\n";
    } else if (percent == 0) {
        std::cout << "Definitely NOT a VM\n";
    } else {
        std::cout << "Unsure if it's a VM\n";
    }

    // converted to int for console character encoding reasons
    std::cout << "percentage: " << static_cast<int>(percent) << "%\n"; 

    return 0;
}