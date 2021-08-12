#pragma once

namespace pepp
{
	template<unsigned int>
	class Image;

	enum SectionCharacteristics : std::uint32_t
	{
        SCN_TYPE_NO_PAD              = 0x00000008,  // Reserved.
        SCN_CNT_CODE                 = 0x00000020,  // Section contains code.
        SCN_CNT_INITIALIZED_DATA     = 0x00000040,  // Section contains initialized data.
        SCN_CNT_UNINITIALIZED_DATA   = 0x00000080,  // Section contains uninitialized data.
        SCN_LNK_OTHER				 = 0x00000100,  // Reserved.
        SCN_LNK_INFO                 = 0x00000200,  // Section contains comments or some other type of information.
        SCN_LNK_REMOVE               = 0x00000800,  // Section contents will not become part of image.
        SCN_LNK_COMDAT               = 0x00001000,  // Section contents comdat.
        SCN_NO_DEFER_SPEC_EXC        = 0x00004000,  // Reset speculative exceptions handling bits in the TLB entries for this section.
        SCN_GPREL                    = 0x00008000,  // Section content can be accessed relative to GP
        SCN_MEM_FARDATA              = 0x00008000,
        SCN_MEM_PURGEABLE            = 0x00020000,
        SCN_MEM_16BIT                = 0x00020000,
        SCN_MEM_LOCKED               = 0x00040000,
        SCN_MEM_PRELOAD              = 0x00080000,
        SCN_ALIGN_1BYTES             = 0x00100000,  //
        SCN_ALIGN_2BYTES             = 0x00200000,  //
        SCN_ALIGN_4BYTES             = 0x00300000,  //
        SCN_ALIGN_8BYTES             = 0x00400000,  //
        SCN_ALIGN_16BYTES            = 0x00500000,  // Default alignment if no others are specified.
        SCN_ALIGN_32BYTES            = 0x00600000,  //
        SCN_ALIGN_64BYTES            = 0x00700000,  //
        SCN_ALIGN_128BYTES           = 0x00800000,  //
        SCN_ALIGN_256BYTES           = 0x00900000,  //
        SCN_ALIGN_512BYTES           = 0x00A00000,  //
        SCN_ALIGN_1024BYTES          = 0x00B00000,  //
        SCN_ALIGN_2048BYTES          = 0x00C00000,  //
        SCN_ALIGN_4096BYTES          = 0x00D00000,  //
        SCN_ALIGN_8192BYTES          = 0x00E00000,  //
        SCN_ALIGN_MASK               = 0x00F00000,
        SCN_LNK_NRELOC_OVFL          = 0x01000000,  // Section contains extended relocations.
        SCN_MEM_DISCARDABLE          = 0x02000000,  // Section can be discarded.
        SCN_MEM_NOT_CACHED           = 0x04000000,  // Section is not cachable.
        SCN_MEM_NOT_PAGED            = 0x08000000,  // Section is not pageable.
        SCN_MEM_SHARED               = 0x10000000,  // Section is shareable.
        SCN_MEM_EXECUTE              = 0x20000000,  // Section is executable.
        SCN_MEM_READ                 = 0x40000000,  // Section is readable.
        SCN_MEM_WRITE                = 0x80000000   // Section is writeable.
	};

	class SectionHeader
	{
		friend class Image<32>;
		friend class Image<64>;
		 
		detail::Image_t<>::SectionHeader_t m_base;
	public:
		SectionHeader() {
			memcpy(m_base.Name, ".dummy", sizeof(".dummy"));
		}

		//! Getter/setter for SectionHeader.Name
		void SetName(std::string_view name);
		std::string GetName() const;

		//! Getter/setters for SectionHeader.Misc
		std::uint32_t GetFileAddress() const;
		void SetFileAddress(std::uint32_t fileAddress);

		std::uint32_t GetVirtualSize() const;
		void SetVirtualSize(std::uint32_t virtualSize);

		//! Getter/setter for VirtualAddress
		std::uint32_t GetVirtualAddress() const;
		void SetVirtualAddress(std::uint32_t va);

		//! Getter/setter for SizeOfRawData
		std::uint32_t GetSizeOfRawData() const;
		void SetSizeOfRawData(std::uint32_t sz);
		 
		//! Getter/setter for SizeOfRawData
		std::uint32_t GetPointerToRawData() const;
		void SetPointerToRawData(std::uint32_t ptr);

		//! Getter/setter for PointerToRelocations
		std::uint32_t GetPointerToRelocations() const;
		void SetPointerToRelocations(std::uint32_t ptr);

		//! Getter/setter for PointerToLinenumbers
		std::uint32_t GetPointerToLinenumbers() const;
		void SetPointerToLinenumbers(std::uint32_t ptr);
		
		//! Getter/setter for NumberOfRelocations
		std::uint16_t GetNumberOfRelocations() const;
		void SetNumberOfRelocations(std::uint16_t num);
		
		//! Getter/setter for 
		std::uint16_t GetNumberOfLinenumbers() const;
		void SetNumberOfLinenumbers(std::uint16_t num);

		//! Getter/setter for 
		std::uint32_t GetCharacteristics() const;
		void SetCharacteristics(std::uint32_t chars);


		// Section utility functions
		void AddCharacteristic(std::uint32_t dwChar) {
			m_base.Characteristics |= dwChar;
		}
		void StripCharacteristic(std::uint32_t dwChar) {
			m_base.Characteristics &= ~dwChar;
		} 
		bool IsReadable() const {
			return GetCharacteristics() & SCN_MEM_READ;
		}
		bool IsWriteable() const {
			return GetCharacteristics() & SCN_MEM_WRITE;
		}
		bool IsExecutable() const {
			return GetCharacteristics() & SCN_MEM_EXECUTE;
		}
		bool HasVirtualAddress(std::uint32_t va) const {
			return va >= m_base.VirtualAddress && va < m_base.VirtualAddress + m_base.Misc.VirtualSize;
		}
		bool HasOffset(std::uint32_t offset) const {
			return offset >= m_base.PointerToRawData && offset < m_base.PointerToRawData + m_base.SizeOfRawData;
		}
	};

	static_assert(sizeof(SectionHeader) == sizeof(detail::Image_t<>::SectionHeader_t), "Invalid size of SectionHeader");
}