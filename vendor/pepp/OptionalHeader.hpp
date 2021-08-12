#pragma once

namespace pepp
{
	enum PEDirectoryEntry
	{
		DIRECTORY_ENTRY_EXPORT          = 0,   // Export Directory
		DIRECTORY_ENTRY_IMPORT          = 1,   // Import Directory
		DIRECTORY_ENTRY_RESOURCE        = 2,   // Resource Directory
		DIRECTORY_ENTRY_EXCEPTION       = 3,   // Exception Directory
		DIRECTORY_ENTRY_SECURITY        = 4,   // Security Directory
		DIRECTORY_ENTRY_BASERELOC       = 5,   // Base Relocation Table
		DIRECTORY_ENTRY_DEBUG           = 6,   // Debug Directory
		DIRECTORY_ENTRY_ARCHITECTURE    = 7,   // Architecture Specific Data
		DIRECTORY_ENTRY_GLOBALPTR       = 8,   // RVA of GP
		DIRECTORY_ENTRY_TLS             = 9,   // TLS Directory
		DIRECTORY_ENTRY_LOAD_CONFIG     = 10,   // Load Configuration Directory
		DIRECTORY_ENTRY_BOUND_IMPORT    = 11,   // Bound Import Directory in headers
		DIRECTORY_ENTRY_IAT             = 12,   // Import Address Table
		DIRECTORY_ENTRY_DELAY_IMPORT    = 13,   // Delay Load Import Descriptors
		DIRECTORY_ENTRY_COM_DESCRIPTOR  = 14    // COM Runtime descriptor
	};

	enum class PEMagic
	{
		HDR_32 = 0x10b,
		HDR_64 = 0x20b,
		HDR_ROM = 0x107
	};

	template<unsigned int bitsize = 32>
	class OptionalHeader : pepp::msc::NonCopyable
	{
		friend class PEHeader<bitsize>;
		friend class Image<bitsize>;

		using ImageData_t = detail::Image_t<bitsize>;

		Image<bitsize>*					m_Image;
		ImageData_t::OptionalHeader_t*	m_base{ nullptr };
	public:
		OptionalHeader();


		//! Getter/setter for OptionalHeader.Magic
		void SetMagic(PEMagic magic);
		PEMagic GetMagic() const;

		//! Getter/setter for OptionalHeader.ImageBase
		void SetImageBase(detail::Image_t<bitsize>::Address_t address);
		detail::Image_t<bitsize>::Address_t GetImageBase() const;

		//! Getter/setter for OptionalHeader.SizeOfImage
		void SetSizeOfImage(std::uint32_t size);
		std::uint32_t GetSizeOfImage() const;

		//! Getter/setter for OptionalHeader.SizeOfCode
		void SetSizeOfCode(std::uint32_t dwSize);
		std::uint32_t GetSizeOfCode() const;

		//! Getter/setter for OptionalHeader.SizeOfInitializedData
		void SetSizeOfInitializedData(std::uint32_t dwSize);
		std::uint32_t GetSizeOfInitializedData() const;

		//! Getter/setter for OptionalHeader.SizeOfUninitializedData
		void SetSizeOfUninitializedData(std::uint32_t dwSize);
		std::uint32_t GetSizeOfUninitializedData() const;

		//! Getter/setter for OptionalHeader.BaseOfCode
		void SetBaseOfCode(std::uint32_t dwBase);
		std::uint32_t GetBaseOfCode() const;

		//! Getter/setter for OptionalHeader.AddressOfEntryPoint
		void SetAddressOfEntryPoint(std::uint32_t dwBase);
		std::uint32_t GetAddressOfEntryPoint() const;

		//! Getter for OptionalHeader.FileAlignment
		std::uint32_t GetFileAlignment() const;

		//! Getter for OptionalHeader.SectionAlignment
		std::uint32_t GetSectionAlignment() const;

		//! Get data directory
		detail::Image_t<>::DataDirectory_t& GetDataDirectory(int idx) const {
			return m_base->DataDirectory[idx];
		}

		//! Calculate the number of directories present (not NumberOfRvaAndSizes)
		std::uint32_t DirectoryCount() const {
			std::uint32_t count{ 0ul };
			for (int i = 0; i < MAX_DIRECTORY_COUNT; i++)
			{
				if (GetDataDirectory(i).Size > 0) {
					++count;
				}
			}
			return count;
		}

		std::uint8_t* base() const {
			return (std::uint8_t*)m_base;
		}

		//! Check if image has relocations
		bool HasRelocations() const;

	private:
		void _setup(Image<bitsize>* image) {
			m_Image = image;
			m_base = &image->GetPEHeader().native()->OptionalHeader;
		}
	};
}