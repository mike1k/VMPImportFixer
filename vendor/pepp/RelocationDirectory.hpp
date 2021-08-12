#pragma once

namespace pepp
{
	/*
	* Relocations: research32.blogspot.com/2015/01/base-relocation-table.html
	* Format looks like
	* 00 10 00 00		| RVA of Block
	* 28 01 00 00		| Size of Block
	* ?? ?? ?? ?? .....	| Entries in block
	* (entry count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOC)) / sizeof(WORD))
	*/
	enum RelocationType : std::int8_t
	{
		REL_BASED_ABSOLUTE           =    0,
		REL_BASED_HIGH               =    1,
		REL_BASED_LOW                =    2,
		REL_BASED_HIGHLOW            =    3,
		REL_BASED_HIGHADJ            =    4,
		REL_BASED_MACHINE_SPECIFIC_5 =    5,
		REL_BASED_RESERVED           =    6,
		REL_BASED_MACHINE_SPECIFIC_7 =    7,
		REL_BASED_MACHINE_SPECIFIC_8 =    8,
		REL_BASED_MACHINE_SPECIFIC_9 =    9,
		REL_BASED_DIR64              =    10
	};

	constexpr std::uint16_t CraftRelocationBlockEntry(RelocationType type, std::uint16_t offset) noexcept {
		return (offset & 0xfff) | (type << 12);
	}

	class BlockEntry
	{
		std::uint32_t m_va;
		std::uint16_t m_entry;
	public:
		BlockEntry(std::uint32_t va, std::uint16_t entry)
			: m_va(va)
			, m_entry(entry)
		{
		}

		RelocationType GetType() const
		{
			return static_cast<RelocationType>(m_entry >> 12);
		}

		std::uint32_t GetOffset() const
		{
			// Single out the last 12 bits of the entry
			return static_cast<std::uint32_t>(m_entry & ((1 << 12) - 1));
		}

		std::uint32_t GetRva() const
		{
			return m_va + GetOffset();
		}

		constexpr operator std::uint16_t() const
		{
			return m_entry;
		}
	};

	class BlockStream
	{
		std::uint16_t*						 m_base;
		std::uint32_t					     m_idx;
		std::uint32_t						 m_max_size;
	public:
		BlockStream(detail::Image_t<>::RelocationBase_t* reloc)
			: m_base((std::uint16_t*)(reloc+1))
			, m_idx(0)
			, m_max_size(reloc->SizeOfBlock)
		{
		}

		void Push(RelocationType type, std::uint16_t offset)
		{
			if (m_idx * sizeof(uint16_t) >= m_max_size)
				return;

			m_base[m_idx++] = CraftRelocationBlockEntry(type, offset);
		}
	};

	template<unsigned int bitsize>
	class RelocationDirectory : pepp::msc::NonCopyable
	{
		friend class Image<32>;
		friend class Image<64>;

		using PatchType_t = typename detail::Image_t<bitsize>::Address_t;

		Image<bitsize>*							m_image;
		detail::Image_t<>::RelocationBase_t*	m_base;
		SectionHeader*							m_section;
	public:

		int			GetNumberOfBlocks() const;
		int			GetNumberOfEntries(detail::Image_t<>::RelocationBase_t* reloc) const;
		std::uint32_t	GetRemainingFreeBytes() const;
		bool			ChangeRelocationType(std::uint32_t rva, RelocationType type);
		std::vector<BlockEntry> GetBlockEntries(int blockIdx);
		BlockStream CreateBlock(std::uint32_t rva, std::uint32_t num_entries);

		bool IsPresent() const {
			return m_image->GetPEHeader().GetOptionalHeader().GetDataDirectory(DIRECTORY_ENTRY_BASERELOC).Size > 0;
		}
	private:
		//! Setup the directory
		void _setup(Image<bitsize>* image) {
			m_image = image;
			m_base = reinterpret_cast<decltype(m_base)>(
				&image->base()[image->GetPEHeader().RvaToOffset(
					image->GetPEHeader().GetOptionalHeader().GetDataDirectory(DIRECTORY_ENTRY_BASERELOC).VirtualAddress)]);
			m_section = 
				&image->GetSectionHeaderFromVa(image->GetPEHeader().GetOptionalHeader().GetDataDirectory(DIRECTORY_ENTRY_BASERELOC).VirtualAddress);
		}
	};
}