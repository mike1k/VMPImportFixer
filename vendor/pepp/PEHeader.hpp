#pragma once

namespace pepp
{
	constexpr static int MAX_DIRECTORY_COUNT = 16;

	template<unsigned int>
	class Image;
	template<unsigned int>
	class OptionalHeader;
	
	class SectionHeader;
	class FileHeader;
	
	template<unsigned int bitsize = 32>
	class PEHeader : pepp::msc::NonCopyable
	{
		friend class Image<bitsize>;

		using ImageData_t = detail::Image_t<bitsize>;

		Image<bitsize>*					m_Image;
		ImageData_t::Header_t*			m_PEHdr = nullptr;
		FileHeader						m_FileHeader;
		OptionalHeader<bitsize>			m_OptionalHeader;
	private:
		//! Private constructor, this should never be established outside of `class Image`
		PEHeader();
	public:

		class FileHeader& GetFileHeader() {
			return m_FileHeader;
		}

		const class FileHeader& GetFileHeader() const {
			return m_FileHeader;
		}

		class OptionalHeader<bitsize>& GetOptionalHeader() {
			return m_OptionalHeader;
		}

		const class OptionalHeader<bitsize>& GetOptionalHeader() const {
			return m_OptionalHeader;
		}

		SectionHeader& GetSectionHeader(std::uint16_t dwIndex) {
			static SectionHeader dummy{};

			if (dwIndex < m_Image->GetNumberOfSections())
				return m_Image->m_rawSectionHeaders[dwIndex];
			
			return dummy;
		}

		SectionHeader& GetSectionHeader(std::string_view name) {
			static SectionHeader dummy{};

			for (std::uint16_t n = 0; n < m_Image->GetNumberOfSections(); n++)
			{
				if (m_Image->m_rawSectionHeaders[n].GetName().compare(name) == 0) {
					return m_Image->m_rawSectionHeaders[n];
				}
			}

			return dummy;
		}

		SectionHeader& GetSectionHeaderFromVa(std::uint32_t va) {
			static SectionHeader dummy{}; 
			
			for (std::uint16_t n = 0; n < m_Image->GetNumberOfSections(); n++)
			{
				if (m_Image->m_rawSectionHeaders[n].HasVirtualAddress(va)) {
					return m_Image->m_rawSectionHeaders[n];
				}
			}

			return dummy;
		}

		SectionHeader& GetSectionHeaderFromOffset(std::uint32_t offset) {
			static SectionHeader dummy{};

			for (std::uint16_t n = 0; n < m_Image->GetNumberOfSections(); n++)
			{
				if (m_Image->m_rawSectionHeaders[n].HasOffset(offset)) {
					return m_Image->m_rawSectionHeaders[n];
				}
			}

			return dummy;
		}



		//! Calculate the number of directories present (not NumberOfRvaAndSizes)
		std::uint32_t DirectoryCount() const {
			return GetOptionalHeader().DirectoryCount();
		}

		//! Convert a relative virtual address to a file offset
		std::uint32_t RvaToOffset(std::uint32_t rva) {
			SectionHeader const& sec { GetSectionHeaderFromVa(rva) };
			//
			// Did we get one?
			if (sec.GetName() != ".dummy") {
				return sec.GetPointerToRawData() + rva - sec.GetVirtualAddress();
			}

			return 0ul;
		}

		//! Convert a file offset back to a relative virtual address
		std::uint32_t OffsetToRva(std::uint32_t offset) {
			SectionHeader const& sec{ GetSectionHeaderFromOffset(offset) };
			//
			// Did we get one?
			if (sec.GetName() != ".dummy") {
				return (sec.GetVirtualAddress() + offset) - sec.GetPointerToRawData();
			}

			return 0ul;
		}
		 
		//! Convert a rel. virtual address to a virtual address
		detail::Image_t<bitsize>::Address_t RvaToVa(std::uint32_t rva) const {
			return m_OptionalHeader.GetImageBase() + rva;
		}

		//! Used to check if the NT tag is present.
		bool IsTaggedPE() const {
			return m_PEHdr->Signature == IMAGE_NT_SIGNATURE;
		}

		std::uint8_t* base() const {
			return (std::uint8_t*)m_PEHdr;
		}

		constexpr std::size_t size() const {
			return sizeof(decltype(*m_PEHdr));
		}

		//! Return native pointer
		detail::Image_t<bitsize>::Header_t* native() {
			return m_PEHdr;
		}

		//! Manually calculate the size of the image
		std::uint32_t GetSizeOfImage();

		//! Manually calculate the start of the code section
		std::uint32_t GetStartOfCode();

		//! Calculate next section offset
		std::uint32_t GetNextSectionOffset();

		//! Calculate next section rva
		std::uint32_t GetNextSectionRva();
	private:
		//! Setup the header
		void _setup(Image<bitsize>* image) {
			m_Image = image;
			m_PEHdr = reinterpret_cast<decltype(m_PEHdr)>(m_Image->base() + m_Image->m_MZHeader->e_lfanew);
			m_FileHeader._setup(image);
			m_OptionalHeader._setup(image);
		}
	};
}