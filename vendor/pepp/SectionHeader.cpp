#include "PELibrary.hpp"

using namespace pepp;

void SectionHeader::SetName(std::string_view name)
{
	std:memcpy(m_base.Name, name.data(), name.size());
}

std::string SectionHeader::GetName() const
{
	char szData[9];
	std::memcpy(szData, m_base.Name, sizeof m_base.Name);
	szData[8] = '\0';
	return szData;
}

std::uint32_t SectionHeader::GetFileAddress() const
{
	return m_base.Misc.PhysicalAddress;
}

void SectionHeader::SetFileAddress(std::uint32_t fileAddress)
{
	m_base.Misc.PhysicalAddress = fileAddress;
}

std::uint32_t SectionHeader::GetVirtualSize() const
{
	return m_base.Misc.VirtualSize;
}

void SectionHeader::SetVirtualSize(std::uint32_t virtualSize)
{
	m_base.Misc.VirtualSize = virtualSize;
}

std::uint32_t SectionHeader::GetVirtualAddress() const
{
	return m_base.VirtualAddress;
}

void SectionHeader::SetVirtualAddress(std::uint32_t va)
{
	m_base.VirtualAddress = va;
}

std::uint32_t SectionHeader::GetSizeOfRawData() const
{
	return m_base.SizeOfRawData;
}

void SectionHeader::SetSizeOfRawData(std::uint32_t sz)
{
	m_base.SizeOfRawData = sz;
}

std::uint32_t SectionHeader::GetPointerToRawData() const
{
	return m_base.PointerToRawData;
}

void SectionHeader::SetPointerToRawData(std::uint32_t ptr)
{
	m_base.PointerToRawData = ptr;
}

std::uint32_t SectionHeader::GetPointerToRelocations() const
{
	return m_base.PointerToRelocations;
}

void SectionHeader::SetPointerToRelocations(std::uint32_t ptr)
{
	m_base.PointerToRelocations = ptr;
}

std::uint32_t SectionHeader::GetPointerToLinenumbers() const
{
	return m_base.PointerToLinenumbers;
}

void SectionHeader::SetPointerToLinenumbers(std::uint32_t ptr)
{
	m_base.PointerToLinenumbers = ptr;
}

std::uint16_t SectionHeader::GetNumberOfRelocations() const
{
	return m_base.NumberOfRelocations;
}

void SectionHeader::SetNumberOfRelocations(std::uint16_t num)
{
	m_base.NumberOfRelocations = num;
}

std::uint16_t SectionHeader::GetNumberOfLinenumbers() const
{
	return m_base.NumberOfLinenumbers;
}

void SectionHeader::SetNumberOfLinenumbers(std::uint16_t num)
{
	m_base.NumberOfLinenumbers = num;
}

std::uint32_t SectionHeader::GetCharacteristics() const
{
	return m_base.Characteristics;
}

void SectionHeader::SetCharacteristics(std::uint32_t chars)
{
	m_base.Characteristics = chars;
}
