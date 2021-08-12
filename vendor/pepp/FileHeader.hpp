#pragma once

namespace pepp
{
	enum class PEMachine
	{
		MACHINE_I386 = 0x14c,
		MACHINE_IA64 = 0x200,
		MACHINE_AMD64 = 0x8664
	};

	class FileHeader : pepp::msc::NonCopyable
	{
		friend class PEHeader<32>;
		friend class PEHeader<64>;

		IMAGE_FILE_HEADER*	m_base;
	public:
		FileHeader() 
		{
		}

		void SetMachine(PEMachine machine) {
			m_base->Machine = static_cast<std::uint16_t>(machine);
		}

		PEMachine GetMachine() const {
			return static_cast<PEMachine>(m_base->Machine);
		}

		void SetNumberOfSections(std::uint16_t numSections) {
			m_base->NumberOfSections = numSections;
		}

		std::uint16_t GetNumberOfSections() const {
			return m_base->NumberOfSections;
		}

		void SetTimeDateStamp(std::uint32_t dwTimeDateStamp) {
			m_base->TimeDateStamp = dwTimeDateStamp;
		}

		std::uint32_t GetTimeDateStamp() const {
			return m_base->TimeDateStamp;
		}

		void SetPointerToSymbolTable(std::uint32_t dwPointerToSymbolTable) {
			m_base->PointerToSymbolTable = dwPointerToSymbolTable;
		}

		std::uint32_t GetPointerToSymbolTable() const {
			return m_base->PointerToSymbolTable;
		}

		void SetNumberOfSymbols(std::uint32_t numSymbols) {
			m_base->NumberOfSymbols = numSymbols;
		}

		std::uint32_t GetNumberOfSymbols() const {
			return m_base->NumberOfSymbols;
		}

		void SetSizeOfOptionalHeader(std::uint16_t size) {
			m_base->SizeOfOptionalHeader = size;
		}

		std::uint16_t GetSizeOfOptionalHeader() const {
			return m_base->SizeOfOptionalHeader;
		}

		void SetCharacteristics(std::uint16_t chars) {
			m_base->Characteristics = chars;
		}

		std::uint16_t GetCharacteristics() const {
			return m_base->Characteristics;
		}

		IMAGE_FILE_HEADER* native() const {
			return m_base;
		}
	private:
		template<unsigned int bitsize>
		void _setup(Image<bitsize>* image) {
			m_base = &image->GetPEHeader().native()->FileHeader;
		}
	};
}