#include "VMPImportFixer.hpp"

DWORD VifSearchForProcess(std::string_view process_name) noexcept
{
    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof pe32;

    vif::nt::ScopedHandle hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (static_cast<HANDLE>(hSnapshot) == INVALID_HANDLE_VALUE)
        return -1;

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            std::string_view this_proc = pe32.szExeFile;
            if (this_proc.find(process_name) != this_proc.npos && pe32.th32ProcessID != GetCurrentProcessId())
                return pe32.th32ProcessID;
        } while (Process32Next(hSnapshot, &pe32));
    }

    return static_cast<DWORD>(-1);
}

bool VifFindModuleInProcess(HANDLE hProc, std::string_view module_name, VIFModuleInformation_t* info)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;

    if (!info)
        return false;

    if (EnumProcessModulesEx(hProc, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
    {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];
            
            if (GetModuleFileNameEx(hProc, hMods[i], szModName,
                sizeof(szModName) / sizeof(TCHAR)))
            {
                MODULEINFO mdi{};
                GetModuleInformation(hProc, hMods[i], &mdi, sizeof mdi);

                info->module_path = szModName;

                if (info->module_path.find(module_name) != std::string::npos)
                {
                    info->base_address = (std::uint64_t)hMods[i];
                    info->module_size = mdi.SizeOfImage;
                }
            }
        }
    }

    return info->base_address != 0;
}

bool VifFindModulesInProcess(HANDLE hProc, std::vector<VIFModuleInformation_t>& modules)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;

    if (EnumProcessModulesEx(hProc, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
    {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.
            if (GetModuleFileNameEx(hProc, hMods[i], szModName,
                sizeof(szModName) / sizeof(TCHAR)))
            {
                MODULEINFO info{};
                GetModuleInformation(hProc, hMods[i], &info, sizeof info);

                modules.emplace_back(szModName, (std::uint64_t)hMods[i], info.SizeOfImage);
            }
        }
    }

    return modules.size() > 0;
}
