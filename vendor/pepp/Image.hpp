#pragma once

namespace pepp
{
	template<unsigned int>
	class PEHeader;
	class SectionHeader;
	template<unsigned int>
	class ExportDirectory;
	template<unsigned int>
	class ImportDirectory;
	template<unsigned int>
	class RelocationDirectory;
	enum SectionCharacteristics;
	enum PEDirectoryEntry;
	enum class PEMachine;

	namespace detail
	{
		template<unsigned int bitsize = 0>
		struct Image_t
		{
			using MZHeader_t = IMAGE_DOS_HEADER;
			using ImportDescriptor_t = IMAGE_IMPORT_DESCRIPTOR;
			using BoundImportDescriptor_t = IMAGE_BOUND_IMPORT_DESCRIPTOR;
			using ResourceDirectory_t = IMAGE_RESOURCE_DIRECTORY;
			using ResourceDirectoryEntry_t = IMAGE_RESOURCE_DIRECTORY_ENTRY;
			using SectionHeader_t = IMAGE_SECTION_HEADER;
			using FileHeader_t = IMAGE_FILE_HEADER;
			using DataDirectory_t = IMAGE_DATA_DIRECTORY;
			using ExportDirectory_t = IMAGE_EXPORT_DIRECTORY;
			using RelocationBase_t = IMAGE_BASE_RELOCATION;
			using ImportAddressTable_t = std::uint32_t;
		};

		template<> struct Image_t<64>
		{
			using Header_t = IMAGE_NT_HEADERS64;
			using TLSDirectory_t = IMAGE_TLS_DIRECTORY64;
			using ThunkData_t = IMAGE_THUNK_DATA64;
			using Address_t = std::uint64_t;
			using OptionalHeader_t = IMAGE_OPTIONAL_HEADER64;
		};

		template<> struct Image_t<32>
		{
			using Header_t = IMAGE_NT_HEADERS32;
			using TLSDirectory_t = IMAGE_TLS_DIRECTORY32;
			using ThunkData_t = IMAGE_THUNK_DATA32;
			using Address_t = std::uint32_t;
			using OptionalHeader_t = IMAGE_OPTIONAL_HEADER32;
		};
	}

	/// 
	//! class Image
	//! Used for runtime or static analysis/manipulating of PE files.
	/// 
	template<unsigned int bitsize = 32>
	class Image : pepp::msc::NonCopyable
	{
		using CPEHeader = const PEHeader<bitsize>;
		using CExportDirectory = const ExportDirectory;
		using CImportDirectory = const ImportDirectory;

	public:

		//! Publicize the detail::Image_t used by this image.
		using ImageData_t = detail::Image_t<bitsize>;

		friend class PEHeader<bitsize>;

		static_assert(bitsize == 32 || bitsize == 64, "Invalid bitsize fed into PE::Image");
	private:	
		detail::Image_t<>::MZHeader_t*			m_MZHeader;
		std::string								m_fileName{};
		mem::ByteVector							m_imageBuffer{};
		PEHeader<bitsize>						m_PEHeader;
		//! Sections
		SectionHeader*							m_rawSectionHeaders;
		//! Exports
		ExportDirectory<bitsize>				m_exportDirectory;
		//! Imports
		ImportDirectory<bitsize>				m_importDirectory;
		//! Relocations
		RelocationDirectory<bitsize>			m_relocDirectory;
		//! Is image mapped? Rva2Offset becomes obsolete
		bool									m_mem_mapped = false;
	public:

		//! Default ctor.
		Image();

		//! Used to construct a `class Image` via a existing file
		Image(std::string_view filepath);
		
		//! Used to construct a `class Image` via a memory buffer
		Image(const void* data, std::size_t size);

		//! Used to construct via another `class Image`
		Image(const Image& image);

		//! 
		[[nodiscard]] static Image FromRuntimeMemory(void* data, std::size_t size) noexcept;
		//! 
		bool SetFromRuntimeMemory(void* data, std::size_t size) noexcept;

		//! Get the start pointer of the buffer.
		std::uint8_t* base() {
			return m_imageBuffer.data();
		}

		mem::ByteVector& buffer() {
			return m_imageBuffer;
		}

		const mem::ByteVector& buffer() const {
			return m_imageBuffer;
		}

		//! Magic number in the DOS header.
		std::uint16_t magic() const {
			return m_MZHeader->e_magic;
		}

		//! PEHeader wrapper
		class PEHeader<bitsize>& GetPEHeader() {
			return m_PEHeader;
		}

		class ExportDirectory<bitsize>& GetExportDirectory() {
			return m_exportDirectory;
		}

		class ImportDirectory<bitsize>& GetImportDirectory() {
			return m_importDirectory;
		}

		class RelocationDirectory<bitsize>& GetRelocationDirectory() {
			return m_relocDirectory;
		}

		const PEHeader<bitsize>& GetPEHeader() const {
			return m_PEHeader;
		}

		const class ExportDirectory<bitsize>& GetExportDirectory() const {
			return m_exportDirectory;
		}

		const class ImportDirectory<bitsize>& GetImportDirectory() const {
			return m_importDirectory;
		}

		const class RelocationDirectory<bitsize>& GetRelocationDirectory() const {
			return m_relocDirectory;
		}

		//! Native pointer
		detail::Image_t<>::MZHeader_t* native() {
			return m_MZHeader;
		}

		//! Write out changes to a new functional image.
		[[nodiscard]] Image<bitsize> Compile();

		//!
		void SetMapped() noexcept;


		//! Get PEMachine
		constexpr PEMachine GetMachine() const;

		//! Is X64
		static constexpr unsigned int GetBitSize() { return bitsize; }

		//! Add a new section to the image
		bool AppendSection(std::string_view sectionName, std::uint32_t size, std::uint32_t chars, SectionHeader* out = nullptr);

		//! Extend an existing section (will break things depending on the section)
		bool ExtendSection(std::string_view sectionName, std::uint32_t delta);

		//! Append a new export
		bool AppendExport(std::string_view exportName, std::uint32_t rva);

		//! Find offset padding of value v with count n, starting at specified header or bottom of image if none specified
		std::uint32_t FindPadding(SectionHeader* s, std::uint8_t v, std::size_t n, std::uint32_t alignment = 0);

		//! Find offset zero padding up to N bytes, starting at specified header or bottom of image if none specified
		std::uint32_t FindZeroPadding(SectionHeader* s, std::size_t n, std::uint32_t alignment = 0);

		//! Find (wildcard acceptable) binary sequence
		std::vector<std::uint32_t> FindBinarySequence(SectionHeader* s, std::string_view binary_seq) const;
		std::vector<std::pair<std::int32_t, std::uint32_t>> FindBinarySequences(SectionHeader* s, std::initializer_list<std::pair<std::int32_t, std::string_view>> binary_seq) const;

		//! Check if a data directory is "present"
		//! - Necessary before actually using the directory
		//!  (e.g not all images will have a valid IMAGE_EXPORT_DIRECTORY)
		bool HasDataDirectory(PEDirectoryEntry entry);

		//! Write out to file
		void WriteToFile(std::string_view filepath);

		//! Wrappers
		SectionHeader& GetSectionHeader(std::uint16_t dwIndex) {
			return m_PEHeader.GetSectionHeader(dwIndex);
		}
		SectionHeader& GetSectionHeader(std::string_view name) {
			return m_PEHeader.GetSectionHeader(name);
		}
		SectionHeader& GetSectionHeaderFromVa(std::uint32_t va) {
			return m_PEHeader.GetSectionHeaderFromVa(va);
		} 
		SectionHeader& GetSectionHeaderFromOffset(std::uint32_t offset) {
			return m_PEHeader.GetSectionHeaderFromOffset(offset);
		}
		std::uint16_t GetNumberOfSections() const {
			return m_PEHeader.GetFileHeader().GetNumberOfSections();
		}

		constexpr auto GetWordSize() const {
			return bitsize == 64 ? sizeof(std::uint64_t) : sizeof(std::uint32_t);
		}

	private:
		//! Setup internal objects/pointers and validate they are proper.
		void _validate();
	};

	using Image64 = Image<64>;
	using Image86 = Image<32>;



}