#pragma once

struct VIFModuleInformation_t
{
    std::string module_path;
    std::uint64_t base_address;
    std::uint32_t module_size;
};


DWORD VifSearchForProcess(std::string_view process_name) noexcept;
bool VifFindModuleInProcess(HANDLE hProc, std::string_view module_name, VIFModuleInformation_t* info);
bool VifFindModulesInProcess(HANDLE hProc, std::vector<VIFModuleInformation_t>& modules);

