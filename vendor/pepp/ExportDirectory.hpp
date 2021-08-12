#pragma once

#include <functional>

namespace pepp
{
	struct ExportData_t
	{
		std::string name{};
		std::uint32_t rva = 0;
		std::uint32_t ordinal = 0xffffffff;
	};

	template<unsigned int bitsize>
	class ExportDirectory : public pepp::msc::NonCopyable
	{
		friend class Image<32>;
		friend class Image<64>;

		Image<bitsize>*							m_image;
		detail::Image_t<>::ExportDirectory_t	*m_base;
	public:
		ExportData_t GetExport(std::uint32_t idx, bool demangle = true) const;
		void AddExport(std::string_view name, std::uint32_t rva);
		void TraverseExports(const std::function<void(ExportData_t*)>& cb_func);
		bool IsPresent() const noexcept;

		void SetNumberOfFunctions(std::uint32_t num) {
			m_base->NumberOfFunctions = num;
		}

		std::uint32_t GetNumberOfFunctions() const {
			return m_base->NumberOfFunctions;
		}

		void SetNumberOfNames(std::uint32_t num) {
			m_base->NumberOfNames = num;
		}

		std::uint32_t GetNumberOfNames() const {
			return m_base->NumberOfNames;
		}

		void SetCharacteristics(std::uint32_t chrs) {
			m_base->Characteristics = chrs;
		}

		std::uint32_t GetCharacteristics() const {
			return m_base->Characteristics;
		}

		void SetTimeDateStamp(std::uint32_t TimeDateStamp) {
			m_base->TimeDateStamp = TimeDateStamp;
		}

		std::uint32_t GetTimeDateStamp() const {
			return m_base->TimeDateStamp;
		}

		void SetAddressOfFunctions(std::uint32_t AddressOfFunctions) {
			m_base->AddressOfFunctions = AddressOfFunctions;
		}

		std::uint32_t GetAddressOfFunctions() const {
			return m_base->AddressOfFunctions;
		}

		void SetAddressOfNames(std::uint32_t AddressOfNames) {
			m_base->AddressOfNames = AddressOfNames;
		}

		std::uint32_t GetAddressOfNames() const {
			return m_base->AddressOfNames;
		}

		void SetAddressOfNameOrdinals(std::uint32_t AddressOfNamesOrdinals) {
			m_base->AddressOfNameOrdinals = AddressOfNamesOrdinals;
		}

		std::uint32_t GetAddressOfNameOrdinals() const {
			return m_base->AddressOfNameOrdinals;
		}


		constexpr std::size_t size() const {
			return sizeof(decltype(*m_base));
		}

	private:
		//! Setup the directory
		void _setup(Image<bitsize>* image) {
			m_image = image;
			m_base = reinterpret_cast<decltype(m_base)>(
				&image->base()[image->GetPEHeader().RvaToOffset(
					image->GetPEHeader().GetOptionalHeader().GetDataDirectory(DIRECTORY_ENTRY_EXPORT).VirtualAddress)]);
		}
	};
}