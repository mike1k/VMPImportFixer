#include "PELibrary.hpp"

using namespace pepp;

// Explicit templates.
template class PEHeader<32>;
template class PEHeader<64>;

template<unsigned int bitsize>
inline PEHeader<bitsize>::PEHeader()
	: m_Image(nullptr)
{
}


template<unsigned int bitsize>
std::uint32_t PEHeader<bitsize>::GetSizeOfImage()
{
	std::uint32_t dwLowestRva{ 0 };
	std::uint32_t dwHighestRva{ 0 };

	for (std::uint16_t n = 0; n < GetFileHeader().GetNumberOfSections(); n++) {
		//
		// Skip sections with bad Misc.VirtualSize
		if (m_Image->m_rawSectionHeaders[n].GetVirtualSize() == 0)
			continue;
		//
		// Fill in high/low rvas if possible.
		if (m_Image->m_rawSectionHeaders[n].GetVirtualAddress() < dwLowestRva)
			dwLowestRva = 
				m_Image->m_rawSectionHeaders[n].GetVirtualAddress();
		if (m_Image->m_rawSectionHeaders[n].GetVirtualAddress() > dwHighestRva)
			dwHighestRva = 
				m_Image->m_rawSectionHeaders[n].GetVirtualAddress() + m_Image->m_rawSectionHeaders[n].GetVirtualSize();
	}

	return (dwHighestRva - dwLowestRva);
}

template<unsigned int bitsize>
std::uint32_t PEHeader<bitsize>::GetStartOfCode()
{
	return m_OptionalHeader.GetBaseOfCode();
}

template<unsigned int bitsize>
std::uint32_t PEHeader<bitsize>::GetNextSectionOffset()
{
	std::uint16_t nlastSecIdx = GetFileHeader().GetNumberOfSections() - 1;
	SectionHeader const& sec = GetSectionHeader(nlastSecIdx);
	std::uint32_t uNextOffset = sec.GetPointerToRawData() + sec.GetSizeOfRawData();

	/*
	* FileAlignment
	* The alignment of the raw data of sections in the image file, in bytes.
	*/
	return Align(uNextOffset, GetOptionalHeader().GetFileAlignment());
}

template<unsigned int bitsize>
std::uint32_t PEHeader<bitsize>::GetNextSectionRva()
{
	std::uint16_t nlastSecIdx = GetFileHeader().GetNumberOfSections() - 1;
	SectionHeader const& sec = GetSectionHeader(nlastSecIdx);
	std::uint32_t uNextRva = sec.GetVirtualAddress() + sec.GetVirtualSize();

	/*
	* SectionAlignment
	* The alignment of sections loaded in memory, in bytes.
	*/
	return Align(uNextRva, GetOptionalHeader().GetSectionAlignment());
}
