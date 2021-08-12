#include "PELibrary.hpp"

using namespace pepp;

template class ExportDirectory<32>;
template class ExportDirectory<64>;

template<unsigned int bitsize>
ExportData_t ExportDirectory<bitsize>::GetExport(std::uint32_t idx, bool demangle /*= true*/) const
{
	if (!IsPresent())
		return {};

	if (idx < GetNumberOfNames()) {
		std::uint8_t* base{};
		std::uint32_t funcAddresses{};
		std::uint32_t funcNames{};
		std::uint32_t funcOrdinals{};
		std::uint32_t funcNamesOffset{};
		mem::ByteVector const* buffer{};

		funcOrdinals = m_image->GetPEHeader().RvaToOffset(GetAddressOfNameOrdinals());
		uint16_t rlIdx = m_image->buffer().deref<uint16_t>(funcOrdinals + (idx * sizeof uint16_t));

		funcAddresses = m_image->GetPEHeader().RvaToOffset(GetAddressOfFunctions() + sizeof(std::uint32_t) * rlIdx);
		funcNames = m_image->GetPEHeader().RvaToOffset(GetAddressOfNames() + sizeof(std::uint32_t) * idx);
		funcNamesOffset = m_image->GetPEHeader().RvaToOffset(m_image->buffer().deref<uint32_t>(funcNames));


		if (funcAddresses && funcNames && funcOrdinals)
		{
			return 
			{
				   demangle ? DemangleName(m_image->buffer().as<char*>(funcNamesOffset)) : m_image->buffer().as<char*>(funcNamesOffset),
				   m_image->buffer().deref<uint32_t>(funcAddresses),
				   rlIdx
			};
		}
	}

	return {};
}

template<unsigned int bitsize>
void ExportDirectory<bitsize>::TraverseExports(const std::function<void(ExportData_t*)>& cb_func)
{
	for (int i = 0; i < GetNumberOfNames(); i++)
	{
		ExportData_t data = GetExport(i);
		if (data.rva != 0)
			cb_func(&data);
	}
}

template<unsigned int bitsize>
bool ExportDirectory<bitsize>::IsPresent() const noexcept
{
	return m_image->GetPEHeader().GetOptionalHeader().GetDataDirectory(DIRECTORY_ENTRY_EXPORT).Size > 0;
}

template<unsigned int bitsize>
void ExportDirectory<bitsize>::AddExport(std::string_view name, std::uint32_t rva)
{
	// TODO
}
