#pragma once

//
// Emulation engine.
#include <unicorn/unicorn.h>

#include <Windows.h>
#include <psapi.h>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <TlHelp32.h>
#include <algorithm>
#include <memory>
#include <inttypes.h>
#include <filesystem>
#pragma comment(lib, "psapi.lib")

//! PE parsing and manipulation and some other utils.
#include "msc/Process.hpp"
#include "msc/ScopedHandle.hpp"
#include <pepp/PELibrary.hpp>

//! Include Zydis disassembler
#include <zydis/include/Zydis/Zydis.h>
#include <zycore/include/Zycore/Format.h>
#include <zycore/include/Zycore/LibC.h>

//! Include spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/stopwatch.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "VIFTools.hpp"

class IVMPImportFixer
{
public:
	virtual ~IVMPImportFixer() = default;
	virtual bool GetModuleFromAddress(std::uintptr_t ptr, VIFModuleInformation_t* mod) = 0;
	virtual void DumpInMemory(HANDLE hProcess, std::string_view sModName) = 0;
	virtual bool GetExportData(std::uintptr_t mod, std::uintptr_t rva, pepp::ExportData_t* exp) = 0;
};

template<size_t BitSize>
class VMPImportFixer : public pepp::msc::NonCopyable, public IVMPImportFixer
{
public:
	VMPImportFixer(std::string_view vmpsn) noexcept;
	
	void DumpInMemory(HANDLE hProcess, std::string_view sModName) final override;

	//! Zydis disassemble an instruction.
	bool DecodeInsn(pepp::Address<> address, ZydisDecodedInstruction& insn) const noexcept;
	std::uintptr_t CalculateAbsoluteAddress(std::uintptr_t runtime_address, ZydisDecodedInstruction& insn) const noexcept;

	bool GetModuleFromAddress(std::uintptr_t ptr, VIFModuleInformation_t* mod) final override;
	bool GetExportData(std::uintptr_t mod, std::uintptr_t rva, pepp::ExportData_t* exp) final override;
private:
	ZydisDecoder						m_decoder;
	std::string							m_strVMPSectionName;
	std::vector<VIFModuleInformation_t>	m_vecModuleList;
	std::vector<pepp::Image<BitSize>>	m_vecImageList;
	std::map<pepp::Address<>, pepp::Image<BitSize>*> m_ImageMap;
};



extern std::shared_ptr<spdlog::logger> logger;

template<size_t BitSize>
inline VMPImportFixer<BitSize>::VMPImportFixer(std::string_view vmpsn) noexcept
	: m_strVMPSectionName(vmpsn)
{
}


