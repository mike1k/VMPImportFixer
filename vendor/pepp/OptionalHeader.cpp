#include "PELibrary.hpp"

using namespace pepp;

// Explicit templates.
template class OptionalHeader<32>;
template class OptionalHeader<64>;

template<unsigned int bitsize>
inline OptionalHeader<bitsize>::OptionalHeader()
{
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetMagic(PEMagic magic)
{
	m_base->Magic = static_cast<std::uint16_t>(magic);
}

template<unsigned int bitsize>
PEMagic OptionalHeader<bitsize>::GetMagic() const
{
	return static_cast<PEMagic>(m_base->Magic);
}


template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetImageBase(detail::Image_t<bitsize>::Address_t address)
{
	m_base->ImageBase = address;
}

template<unsigned int bitsize>
detail::Image_t<bitsize>::Address_t OptionalHeader<bitsize>::GetImageBase() const
{
	return m_base->ImageBase;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetSizeOfImage(std::uint32_t size)
{
	m_base->SizeOfImage = size;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetSizeOfImage() const
{
	return m_base->SizeOfImage;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetSizeOfCode(std::uint32_t dwSize)
{
	m_base->SizeOfCode = dwSize;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetSizeOfCode() const
{
	return m_base->SizeOfCode;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetSizeOfInitializedData(std::uint32_t dwSize)
{
	m_base->SizeOfInitializedData = dwSize;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetSizeOfInitializedData() const
{
	return m_base->SizeOfInitializedData;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetSizeOfUninitializedData(std::uint32_t dwSize)
{
	m_base->SizeOfUninitializedData = dwSize;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetSizeOfUninitializedData() const
{
	return m_base->SizeOfUninitializedData;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetBaseOfCode(std::uint32_t dwBase)
{
	m_base->BaseOfCode = dwBase;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetBaseOfCode() const
{
	return m_base->BaseOfCode;
}

template<unsigned int bitsize>
void OptionalHeader<bitsize>::SetAddressOfEntryPoint(std::uint32_t dwBase)
{
	m_base->AddressOfEntryPoint = dwBase;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetAddressOfEntryPoint() const
{
	return m_base->AddressOfEntryPoint;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetFileAlignment() const
{
	return m_base->FileAlignment;
}

template<unsigned int bitsize>
std::uint32_t OptionalHeader<bitsize>::GetSectionAlignment() const
{
	return m_base->SectionAlignment;
}

template<unsigned int bitsize>
bool OptionalHeader<bitsize>::HasRelocations() const
{
	return m_base->DataDirectory[DIRECTORY_ENTRY_BASERELOC].Size > 0;
}
