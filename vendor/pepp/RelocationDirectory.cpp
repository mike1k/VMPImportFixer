#include "PELibrary.hpp"

using namespace pepp;

// Explicit templates.
template class RelocationDirectory<32>;
template class RelocationDirectory<64>;

template<unsigned int bitsize>
int RelocationDirectory<bitsize>::GetNumberOfBlocks() const
{
	auto base = m_base;
	int count = 0;

	while (base->VirtualAddress)
	{
		count++;
		base = decltype(base)((char*)base + base->SizeOfBlock);
	}

	return count;
}

template<unsigned int bitsize>
int RelocationDirectory<bitsize>::GetNumberOfEntries(detail::Image_t<>::RelocationBase_t* reloc) const
{
	// MSDN: The Block Size field is then followed by any number of Type or Offset field entries.
	// Each entry is a WORD (2 bytes)
	return (reloc->SizeOfBlock - sizeof(decltype(*reloc))) / sizeof(std::uint16_t);
}

template<unsigned int bitsize>
std::uint32_t RelocationDirectory<bitsize>::GetRemainingFreeBytes() const
{
	auto base = m_base;
	std::uint32_t count = 0;

	while (base->VirtualAddress)
	{
		count += base->SizeOfBlock;
		base = decltype(base)((char*)base + base->SizeOfBlock);
	}

	return std::max<std::uint32_t>(m_section->GetVirtualSize() - count, 0);
}

template<unsigned int bitsize>
bool pepp::RelocationDirectory<bitsize>::ChangeRelocationType(std::uint32_t rva, RelocationType type)
{
	auto base = m_base;
	std::vector<BlockEntry> entries;

	while (base->VirtualAddress)
	{
		int numEntries = GetNumberOfEntries(base);
		std::uint16_t* entry = (std::uint16_t*)(base + 1);

		for (int i = 0; i != numEntries; i++, entry++)
		{
			BlockEntry block(base->VirtualAddress, *entry);
			if (block.GetRva() == rva)
			{
				*entry = CraftRelocationBlockEntry(type, block.GetOffset());
				return true;
			}
		}

		base = decltype(base)((char*)base + base->SizeOfBlock);
	}

	return false;
}

template<unsigned int bitsize>
std::vector<BlockEntry> RelocationDirectory<bitsize>::GetBlockEntries(int blockIdx)
{
	auto base = m_base;
	int count = 0;
	std::vector<BlockEntry> entries;

	while (base->VirtualAddress)
	{
		if (count == blockIdx)
		{
			int numEntries = GetNumberOfEntries(base);
			std::uint16_t* entry = (std::uint16_t*)(base + 1);

			for (int i = 0; i != numEntries; i++, entry++)
			{
				entries.emplace_back(base->VirtualAddress, *entry);
			}
		}

		base = decltype(base)((char*)base + base->SizeOfBlock);
		count++;
	}

	return entries;
}

template<unsigned int bitsize>
BlockStream RelocationDirectory<bitsize>::CreateBlock(std::uint32_t rva, std::uint32_t num_entries)
{
	std::uint32_t size = sizeof(detail::Image_t<>::RelocationBase_t) + num_entries * sizeof(std::uint16_t);

	detail::Image_t<>::RelocationBase_t* reloc;

	std::uint32_t remaining_bytes = GetRemainingFreeBytes();
	if (remaining_bytes < size)
	{
		assert(m_image->ExtendSection(m_section->GetName(), size - remaining_bytes));
	}

	auto base = m_base;
	int count = 0;

	while (base->VirtualAddress)
	{
		base = decltype(base)((char*)base + base->SizeOfBlock);
	}

	base->VirtualAddress = rva;
	base->SizeOfBlock = size;

	return BlockStream(base);
}