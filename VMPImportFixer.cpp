#include "VMPImportFixer.hpp"



std::shared_ptr<spdlog::logger> logger;

// Explicit templates.
template class VMPImportFixer<32>;
template class VMPImportFixer<64>;

class UnicornEngine
{
	uc_engine* ptr;
public:
	UnicornEngine(uc_engine* eng) noexcept
		: ptr(eng)
	{}

	~UnicornEngine() noexcept
	{
		if (ptr)
			uc_close(ptr);
	}
};


template<size_t BitSize>
inline void VMPImportFixer<BitSize>::DumpInMemory(HANDLE hProcess, std::string_view sModName)
{
	//
	// Define types for the current mode.
	using AddressType = pepp::detail::Image_t<BitSize>::Address_t;
	using Address = pepp::Address<AddressType>;

	static constexpr uc_mode EMULATION_MODE = BitSize == 32 ? UC_MODE_32 : UC_MODE_64;
	static constexpr uc_x86_reg STACK_REGISTER = BitSize == 32 ? UC_X86_REG_ESP : UC_X86_REG_RSP;
	static ZydisMachineMode ZY_MACHINE_MODE = BitSize == 32 ? ZYDIS_MACHINE_MODE_LONG_COMPAT_32 : ZYDIS_MACHINE_MODE_LONG_64;
	static ZydisAddressWidth ZY_ADDRESS_WIDTH = BitSize == 32 ? ZYDIS_ADDRESS_WIDTH_32 : ZYDIS_ADDRESS_WIDTH_64;

	//
	// Initialize unicorn.
	uc_engine* uc{};
	uc_hook code_hook{};
	uc_err err = uc_open(UC_ARCH_X86, EMULATION_MODE, &uc);
	UnicornEngine _scoped_unicorn_free(uc);

	if (err != UC_ERR_OK)
	{
		logger->critical("Unable to open Unicorn in X86-{} mode (err: {})", BitSize, err);
		return;
	}

	if (ZyanStatus zs; !ZYAN_SUCCESS((zs = ZydisDecoderInit(&m_decoder, ZY_MACHINE_MODE, ZY_ADDRESS_WIDTH))))
	{
		logger->critical("Unable to initialize Zydis (err: {:X})", BitSize, zs);
		return;
	}

	vif::nt::Process proc(hProcess);
	pepp::Image<BitSize>* pTargetImg = nullptr;
	

	if (proc.handle() == INVALID_HANDLE_VALUE)
	{
		return;
	}


	if (!VifFindModulesInProcess(hProcess, m_vecModuleList) || m_vecModuleList.empty())
	{
		logger->critical("Unable to fetch module list from process.");
		return;
	}

	int Idx{}, mIdx{};

	for (auto& mod : m_vecModuleList)
	{
		size_t nLastSize = 0;
		std::unique_ptr<std::uint8_t> pModBuffer(new std::uint8_t[mod.module_size]{});
		MEMORY_BASIC_INFORMATION mbi{};

		//
		// Loop through the module's memory and insert into the buffer.
		while (VirtualQueryEx(proc.handle(), (PVOID)(mod.base_address + nLastSize), &mbi, sizeof (mbi)))
		{
			if (proc.ReadMemory(mbi.BaseAddress, &pModBuffer.get()[nLastSize], mbi.RegionSize))
				; // logger->info("Read memory at {} with size {}", mbi.BaseAddress, mbi.RegionSize);
			else
				// Log the faliure, but that is all. We will still try to parse.
				logger->critical("Unable to read memory at {:X}", (std::uintptr_t)mbi.BaseAddress);

			nLastSize += mbi.RegionSize;

			if (nLastSize >= mod.module_size)
				break;
		}

		logger->info("Pushing module {} located @ 0x{:X}", mod.module_path, mod.base_address);

		m_vecImageList.emplace_back(std::move(pepp::Image<BitSize>::FromRuntimeMemory(pModBuffer.get(), mod.module_size)));

		if (m_vecImageList.back().magic() != IMAGE_DOS_SIGNATURE)
		{
			logger->error("Failed parsing image: {}", mod.module_path);
			continue;
		}

		if (!sModName.empty() && mod.module_path.find(sModName) != std::string::npos)
			mIdx = Idx;

		++Idx;
	}

	for (auto &img : m_vecImageList)
	{
		if (!sModName.empty())
			pTargetImg = &m_vecImageList[mIdx];

		m_ImageMap[img.GetPEHeader().GetOptionalHeader().GetImageBase()] = &img;
	}

	//
	// If no target module is selected, we default to the base process.
	if (pTargetImg == nullptr)
		pTargetImg = &m_vecImageList.front();


	Address uImageBase = pTargetImg->GetPEHeader().GetOptionalHeader().GetImageBase();

	logger->info("Using base address: {:X}", uImageBase.uintptr());

	//
	// By default, we scan the .text section by name. If the target binary for whatever reason
	// has another name other than .text for it's code section, you will need to change this.
	pepp::SectionHeader& secText = pTargetImg->GetSectionHeader(".text");
	
	if (secText.GetName() == ".dummy")
	{
		logger->critical("Unable to find .text section!");
		return;
	}

	logger->info("Found .text section at virtual address {:X}", secText.GetVirtualAddress());

	pepp::SectionHeader& secVMP = pTargetImg->GetSectionHeader(m_strVMPSectionName);
	if (secVMP.GetName() == ".dummy")
	{
		logger->critical("Unable to find {} section!", secVMP.GetName());
		return;
	}

	logger->info("Found {} section at virtual address {:X}", m_strVMPSectionName, secVMP.GetVirtualAddress());

	//
	// Find all call sequences in the .text section.
	std::vector<uint32_t> vecCallMatches = 
		pTargetImg->FindBinarySequence(&secText, "E8 ? ? ? ?");

	if (vecCallMatches.empty())
	{
		logger->critical("Unable to find any call/jmp sequences in the .text section!");
		return;
	}

	//
    // Map the .text and .vmp0 sections into the emulator memory (even more overhead, but necessary)
	Address uMappedTextAddress = (uImageBase + secText.GetVirtualAddress());
	Address uMappedTextSize = pepp::Align4kb(secText.GetVirtualSize() + 0x1000);

	err = uc_mem_map(uc, uMappedTextAddress.uintptr(), uMappedTextSize.uintptr(), UC_PROT_ALL);
	if (err != UC_ERR_OK)
	{
		logger->critical("Could not map in .text section => uc_mem_map() failed with error: {}", err);
		return;
	}

	err = uc_mem_write(uc, uMappedTextAddress.uintptr(), &pTargetImg->buffer()[secText.GetVirtualAddress()], secText.GetVirtualSize());
	if (err != UC_ERR_OK)
	{
		logger->critical("Could not map in .text section => uc_mem_write() failed with error: {}", err);
		return;
	}

	Address uMappedVmpAddress = (uImageBase + secVMP.GetVirtualAddress());
	Address uMappedVmpSize = pepp::Align4kb(secVMP.GetVirtualSize() + 0x1000);

	err = uc_mem_map(uc, uMappedVmpAddress.uintptr(), uMappedVmpSize.uintptr(), UC_PROT_ALL);
	if (err != UC_ERR_OK)
	{
		logger->critical("Could not map in VMP section => uc_mem_map() failed with error: {}", err);
		return;
	}

	err = uc_mem_write(uc, uMappedVmpAddress.uintptr(), &pTargetImg->buffer()[secVMP.GetVirtualAddress()], secVMP.GetVirtualSize());
	if (err != UC_ERR_OK)
	{
		logger->critical("Could not map in VMP section => uc_mem_write() failed with error: {}", err);
		return;
	}

	//
    // Write the stack address and registers
	auto STACK_SPACE = (uMappedVmpAddress.uintptr() + (uMappedVmpSize.uintptr() - 0x1000)) & -0x10;
	uc_reg_write(uc, STACK_REGISTER, &STACK_SPACE);

	//
	// Temp data to hold info about resolved imports..
	static std::pair<std::string, pepp::ExportData_t> ExpResolved{};

	//
	// We need to monitor every instruction that executes (since it seems like we cannot hook the 
	// exact instruction we need (RET))
	auto VifCodeHook = +[](uc_engine* uc, uint64_t address, uint32_t size, void* user_data)
	{
		IVMPImportFixer* pUd = (IVMPImportFixer*)user_data;

		uint8_t insnbuf[0xf];
		uc_mem_read(uc, address, insnbuf, size);

		ExpResolved.first.clear();

		//
		// Did we hit a RET?
		if (insnbuf[0] == 0xC3 || insnbuf[0] == 0xC2)
		{
			//
			// Real import address is stored in [sp reg]
			AddressType uImportAddress{};
			VIFModuleInformation_t mod{};
			pepp::ExportData_t exp{};

			uc_reg_read(uc, STACK_REGISTER, &uImportAddress);
			uc_mem_read(uc, uImportAddress, &uImportAddress, sizeof(uImportAddress));

			if (pUd->GetModuleFromAddress(uImportAddress, &mod))
			{
				if (!pUd->GetExportData(mod.base_address, uImportAddress - mod.base_address, &ExpResolved.second))
				{
					logger->critical("Could not find export from address {:X}", uImportAddress);
					return;
				}

				ExpResolved.first = std::filesystem::path(mod.module_path).filename().string();

				// logger->info("Resolved a call to {}!{}", ExpResolved.first, ExpResolved.second.name);
				
				//
				// Stop emulation so we don't get a memory fetch error.
				uc_emu_stop(uc);
			}
			else
			{
				logger->critical("Could not find module from address {:X}", uImportAddress);
				return;
			}
		}
	};


	if ((err=uc_hook_add(uc,
		&code_hook,
		UC_HOOK_CODE,
		VifCodeHook,
		this,
		1,
		0)) != UC_ERR_OK)
	{
		logger->critical("Could not install a code hook: {}", err);
		return;
	}

	//
	// Locations of vmp import calls
	std::vector<std::pair<Address, Address>> vecVmpImportCalls{};
	//
	// Cache of imports that were added.
	std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> mAddedImports;

	for (auto match : vecCallMatches)
	{
		ZydisDecodedInstruction insn{};
		std::uint8_t* insnbuf = &pTargetImg->buffer()[match];

		if (DecodeInsn(insnbuf, insn))
		{
			AddressType uDestAddress 
				= CalculateAbsoluteAddress((uImageBase.uintptr() + match), insn);
			if (uDestAddress == 0)
				continue;

			if (secVMP.HasVirtualAddress(uDestAddress - uImageBase.uintptr()))
			{
				logger->info("Found call to {} in {} @ {:X} (call to {:X})",
					m_strVMPSectionName,
					".text",
					(AddressType)(uImageBase + match).uintptr(),
					uDestAddress);

				vecVmpImportCalls.emplace_back((uint64_t)match, uDestAddress);
			}
		}
	}

	for (auto& address : vecVmpImportCalls)
	{
		//
		// Reset stack.
		uc_reg_write(uc, STACK_REGISTER, &STACK_SPACE);

		//
		// Write the return address as if we just entered a CALL.
		uintptr_t stackptr{};
		uintptr_t rtnaddress{ uImageBase.uintptr() + address.first.uintptr() + 5 };

		uc_reg_read(uc, STACK_REGISTER, &stackptr);
		uc_mem_write(uc, stackptr, &rtnaddress, sizeof(rtnaddress));

		// logger->info("Starting emulation @ {:X}", address.second.uintptr());

		//
		// Begin emulation.
		uc_err uerr = uc_emu_start(uc, address.second.uintptr(), 0, 0, 0);
		if (uerr != UC_ERR_OK)
		{
			logger->error("Emulation failed with error: {}", uerr);
			continue;
		}

		if (ExpResolved.first.empty())
		{
			logger->error("Failed to resolve import @ emu address {:X}", address.second.uintptr());
			continue;
		}

		std::uint32_t uImportRVA{};
		std::uint64_t uImportVA{};

		if (mAddedImports.find(ExpResolved.first) != mAddedImports.end() && mAddedImports[ExpResolved.first][ExpResolved.second.name])
		{
			uImportRVA = mAddedImports[ExpResolved.first][ExpResolved.second.name];
		}
		else
		{
			if (!pTargetImg->GetImportDirectory().HasModuleImport(ExpResolved.first, ExpResolved.second.name, &uImportRVA))
				pTargetImg->GetImportDirectory().AddModuleImport(ExpResolved.first, ExpResolved.second.name, &uImportRVA);
			mAddedImports[ExpResolved.first][ExpResolved.second.name] = uImportRVA;
		}

		uImportVA = uImageBase.uintptr() + uImportRVA;

		if (pTargetImg->buffer().deref<uint8_t>(address.first.uintptr() + 5) == 0xcc ||
			pTargetImg->buffer().deref<uint8_t>(address.first.uintptr() + 5) == 0xc3)
		{
			std::uint8_t patch_buf[6];
			patch_buf[0] = 0xff;
			patch_buf[1] = 0x15;
			if constexpr (BitSize == 64)
				*(std::uint32_t*)(&patch_buf[2]) = (std::uint32_t)(uImportVA - (uImageBase.uintptr() + address.first.uintptr()) - 6);
			else
			{
				*(std::uint32_t*)(&patch_buf[2]) = (std::uint32_t)(uImportVA);
			}
			//
			// Patch in
			pTargetImg->buffer().copy_data(
				address.first.uintptr(),
				patch_buf,
				sizeof(patch_buf)
			);

			logger->info("Patched import call @ 0x{:X} to {}!{}",
				address.first.uintptr(),
				ExpResolved.first,
				ExpResolved.second.name);
		}
		else
		{
			//
			// push/call sequence
			std::uint8_t patch_buf[6];
			patch_buf[0] = 0xff;
			patch_buf[1] = 0x15;
			if constexpr (BitSize == 64)
				*(std::uint32_t*)(&patch_buf[2]) = (std::uint32_t)(uImportVA - (uImageBase.uintptr() + (address.first.uintptr() - 1)) - 6);
			else
				*(std::uint32_t*)(&patch_buf[2]) = (std::uint32_t)(uImportVA);

			//
			// Patch in
			pTargetImg->buffer().copy_data(
				address.first.uintptr() - 1,
				patch_buf,
				sizeof(patch_buf)
			);

			logger->info("Patched import call @ 0x{:X} to {}!{}",
				address.first.uintptr(),
				ExpResolved.first,
				ExpResolved.second.name);
		}
	}

	std::string outpath = "dumps/";
	if (sModName.empty()) 
	{
		outpath += std::filesystem::path(m_vecModuleList[0].module_path).filename().string() + ".fixed";
	}
	else
	{
		outpath += std::string(sModName) + ".fixed";
	}

	logger->info("Finished, writing to {}", outpath);

	pTargetImg->WriteToFile(outpath);
}

template<size_t BitSize>
bool VMPImportFixer<BitSize>::DecodeInsn(pepp::Address<> address, ZydisDecodedInstruction& insn) const noexcept
{
	return ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&m_decoder, address.as_ptr<void>(), 0xff, &insn));
}

template<size_t BitSize>
std::uintptr_t VMPImportFixer<BitSize>::CalculateAbsoluteAddress(std::uintptr_t runtime_address, ZydisDecodedInstruction& insn) const noexcept
{
	std::uintptr_t result{};

	if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&insn, &insn.operands[0], runtime_address, &result)))
		return result;

	return 0ull;
}

template<size_t BitSize>
bool VMPImportFixer<BitSize>::GetModuleFromAddress(std::uintptr_t ptr, VIFModuleInformation_t* pmod)
{
	if (m_vecModuleList.empty())
		return false;

	for (auto& mod : m_vecModuleList)
	{
		if (ptr >= mod.base_address && ptr <= mod.base_address + mod.module_size)
		{
			*pmod = mod;
			return true;
		}
	}

	return false;
}

template<size_t BitSize>
bool VMPImportFixer<BitSize>::GetExportData(std::uintptr_t mod, std::uintptr_t rva, pepp::ExportData_t* exp)
{
	pepp::Image<BitSize>* pImage = m_ImageMap[mod];
	bool bFound = false;

	if (pImage)
	{
		pImage->GetExportDirectory().TraverseExports([&bFound, &exp, rva](pepp::ExportData_t* tmp)
			{
				if (tmp->rva == rva)
				{
					*exp = *tmp;
					bFound = true;
				}
			});
	}

	return bFound;
}
