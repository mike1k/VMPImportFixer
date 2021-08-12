#include "PELibrary.hpp"
#include "PEUtil.hpp"
#include <algorithm>

using namespace pepp;

// Explicit templates.
template class Image<32>;
template class Image<64>;

template<unsigned int bitsize>
Image<bitsize>::Image()
{
}

template<unsigned int bitsize>
Image<bitsize>::Image(const Image& rhs)
	: m_fileName(rhs.m_fileName)
	//, m_imageBuffer(std::move(rhs.m_imageBuffer)) -- bad
	, m_imageBuffer(rhs.m_imageBuffer)
{
	// Ensure that the file was read.
	assert(m_imageBuffer.size() > 0);

	// Validate there is a valid MZ signature.
	_validate();
}

template<unsigned int bitsize>
constexpr PEMachine Image<bitsize>::GetMachine() const
{
	if constexpr (bitsize == 32)
		return PEMachine::MACHINE_I386;

	return PEMachine::MACHINE_AMD64;
}


template<unsigned int bitsize>
Image<bitsize>::Image(std::string_view filepath)
	: m_fileName(filepath)
{
	io::File file(m_fileName, io::FILE_INPUT | io::FILE_BINARY);

	std::vector<uint8_t> data{ file.Read() };

	m_imageBuffer.resize(data.size());
	m_imageBuffer.copy_data(0, data.data(), data.size());

	// Ensure that the file was read.
	assert(m_imageBuffer.size() > 0);

	// Validate there is a valid MZ signature.
	_validate();
}

template<unsigned int bitsize>
Image<bitsize>::Image(const void* data, std::size_t size)
{
	m_imageBuffer.resize(size);
	std::memcpy(&m_imageBuffer[0], data, size);

	// Validate there is a valid MZ signature.
	_validate();
}

template<unsigned int bitsize>
[[nodiscard]] Image<bitsize> Image<bitsize>::FromRuntimeMemory(void* data, std::size_t size) noexcept
{
	Image<bitsize> _r;

	_r.m_MZHeader = reinterpret_cast<detail::Image_t<>::MZHeader_t*>(data);

	// Valid MZ tag?
	assert(_r.magic() == IMAGE_DOS_SIGNATURE);

	// Setup the PE header data.
	_r.m_PEHeader._setup(&_r);

	assert(_r.m_PEHeader.IsTaggedPE());

	_r.m_imageBuffer.resize(size);
	std::memcpy(&_r.m_imageBuffer[0], data, _r.m_imageBuffer.size());

	//
	// Okay just _validate now.
	_r._validate();

	//
	// It's runtime, so map it.
	_r.SetMapped(); 
	_r._validate();


	return _r;
}

template<unsigned int bitsize>
bool Image<bitsize>::SetFromRuntimeMemory(void* data, std::size_t size) noexcept
{
	m_MZHeader = reinterpret_cast<detail::Image_t<>::MZHeader_t*>(data);

	// Valid MZ tag?
	assert(magic() == IMAGE_DOS_SIGNATURE);

	// Setup the PE header data.
	m_PEHeader._setup(this);

	assert(m_PEHeader.IsTaggedPE());

	m_imageBuffer.resize(size);
	std::memcpy(&m_imageBuffer[0], data, m_imageBuffer.size());

	//
	// Okay just _validate now.
	_validate();

	//
	// It's runtime, so map it.
	SetMapped();
	_validate();


	return true;
}

template<unsigned int bitsize>
bool Image<bitsize>::HasDataDirectory(PEDirectoryEntry entry)
{
	return GetPEHeader().GetOptionalHeader().GetDataDirectory(entry).Size > 0;
}

template<unsigned int bitsize>
void Image<bitsize>::WriteToFile(std::string_view filepath)
{
	io::File file(filepath, io::FILE_OUTPUT | io::FILE_BINARY);
	file.Write(m_imageBuffer);
}

template<unsigned int bitsize>
void Image<bitsize>::_validate()
{
	m_MZHeader = reinterpret_cast<detail::Image_t<>::MZHeader_t*>(base());

	// Valid MZ tag?
	assert(magic() == IMAGE_DOS_SIGNATURE);

	// Setup the PE header data.
	m_PEHeader._setup(this);

	assert(m_PEHeader.IsTaggedPE());

	// Setup sections
	m_rawSectionHeaders = (m_PEHeader.m_PEHdr ? reinterpret_cast<SectionHeader*>(IMAGE_FIRST_SECTION(m_PEHeader.m_PEHdr)) : nullptr);

	assert(m_rawSectionHeaders != nullptr);

	//
	// Ensure the Image class was constructed with the correct bitsize.
	if constexpr (bitsize == 32)
	{
		assert(m_PEHeader.GetOptionalHeader().GetMagic() == PEMagic::HDR_32);
	}
	else if constexpr (bitsize == 64)
	{
		assert(m_PEHeader.GetOptionalHeader().GetMagic() == PEMagic::HDR_64);
	}

	//
	// Setup export directory
	m_exportDirectory._setup(this);
	//
	// Setup import directory
	m_importDirectory._setup(this);
	//
	// Setup reloc directory
	m_relocDirectory._setup(this);
}

template<unsigned int bitsize>
bool Image<bitsize>::AppendExport(std::string_view exportName, std::uint32_t rva)
{
	GetExportDirectory().AddExport(exportName, rva);
	return false;
}

template<unsigned int bitsize>
bool Image<bitsize>::ExtendSection(std::string_view sectionName, std::uint32_t delta)
{
	std::uint32_t fileAlignment = GetPEHeader().GetOptionalHeader().GetFileAlignment();
	std::uint32_t sectAlignment = GetPEHeader().GetOptionalHeader().GetSectionAlignment();
	if (fileAlignment == 0 || sectAlignment == 0 || delta == 0)
		return false;

	SectionHeader& header = GetSectionHeader(sectionName);

	if (header.GetName() != ".dummy")
	{
		std::unique_ptr<uint8_t> zero_buf(new uint8_t[delta]{});


		header.SetSizeOfRawData(header.GetSizeOfRawData() + delta);
		header.SetVirtualSize(header.GetVirtualSize() + delta);

		for (int i = 0; i < MAX_DIRECTORY_COUNT; i++)
		{
			auto& dir = GetPEHeader().GetOptionalHeader().GetDataDirectory(i);

			if (dir.VirtualAddress == header.GetVirtualAddress())
			{
				dir.Size = header.GetVirtualSize();
				break;
			}
		}

		//
		// Update image size
		GetPEHeader().GetOptionalHeader().SetSizeOfImage(GetPEHeader().GetOptionalHeader().GetSizeOfImage() + delta);

		//
		// Fill in data
		buffer().insert_data(header.GetPointerToRawData() + header.GetSizeOfRawData(), zero_buf.get(), delta);

		//
		// Re-validate the image/headers.
		_validate();

		return true;
	}

	return false;
}

template<unsigned int bitsize>
std::uint32_t Image<bitsize>::FindPadding(SectionHeader* s, std::uint8_t v, std::size_t n, std::uint32_t alignment)
{
	bool bTraverseUp = s == nullptr;
	std::uint32_t startOffset{};

	n = pepp::Align(n, alignment);

	if (s == nullptr)
		s = &m_rawSectionHeaders[GetNumberOfSections() - 1];

	startOffset = s->GetPointerToRawData();

	std::vector<uint8_t>::iterator it = buffer().end();

	if (bTraverseUp)
	{
		//
		std::vector<uint8_t> tmpData(n, v);

		for (std::uint32_t i = startOffset + s->GetSizeOfRawData(); i > n; i = Align(i - n, alignment))
		{
			if (memcmp(&buffer()[i - n], tmpData.data(), tmpData.size()) == 0)
			{
				it = buffer().begin() + (i - n);
				break;
			}
		}
	}
	else
	{
		std::vector<uint8_t> tmpData(n, v);

		for (std::uint32_t i = startOffset; i < startOffset + (buffer().size() - startOffset); i = Align(i + n, alignment))
		{
			if (memcmp(&buffer()[i], tmpData.data(), tmpData.size()) == 0)
			{
				it = buffer().begin() + (i);
				break;
			}
		}
	}


	if (it == buffer().end())
		return -1;

	return (std::uint32_t)std::distance(buffer().begin(), it);
}

template<unsigned int bitsize>
std::uint32_t Image<bitsize>::FindZeroPadding(SectionHeader* s, std::size_t n, std::uint32_t alignment)
{
	return FindPadding(s, 0x0, n, alignment);
}

template<unsigned int bitsize>
std::vector<std::uint32_t> Image<bitsize>::FindBinarySequence(SectionHeader* s, std::string_view binary_seq) const
{
	constexpr auto ascii_to_byte = [](const char ch) [[msvc::forceinline]] {
				if (ch >= '0' && ch <= '9')
					return std::uint8_t(ch - '0');
				if (ch >= 'A' && ch <= 'F')
					return std::uint8_t(ch - 'A' + '\n');
				return std::uint8_t(ch - 'a' + '\n');
	};

	std::vector<std::uint32_t> offsets{};

	if (s == nullptr)
		s = &m_rawSectionHeaders[GetNumberOfSections() - 1];

	std::uint32_t start_offset = s->GetPointerToRawData();
	std::uint32_t result = 0;
	std::uint32_t match_count = 0;

	for (std::uint32_t i = start_offset; i <= start_offset + s->GetSizeOfRawData(); ++i)
	{
		for (int c = 0; c < binary_seq.size();)
		{
			if (binary_seq[c] == ' ')
			{
				++c;
				continue;
			}

			if (binary_seq[c] == '?')
			{
				++c;
				++match_count;
				continue;
			}

			if (buffer()[i + match_count++] != ((ascii_to_byte(binary_seq[c]) << 4) | ascii_to_byte(binary_seq[c + 1])))
			{
				result = 0;
				break;
			}

			result = i;
			c += 2;
		}
	
		if (result)
		{
			offsets.emplace_back(i);
			i += match_count - 1;
		}

		
		match_count = 0;
		result = 0;
	}

	return offsets;
}

template<unsigned int bitsize>
std::vector<std::pair<std::int32_t, std::uint32_t>> Image<bitsize>::FindBinarySequences(SectionHeader* s, std::initializer_list<std::pair<std::int32_t, std::string_view>> binary_seq) const
{
	constexpr auto ascii_to_byte = [](const char ch) [[msvc::forceinline]] {
				if (ch >= '0' && ch <= '9')
					return std::uint8_t(ch - '0');
				if (ch >= 'A' && ch <= 'F')
					return std::uint8_t(ch - 'A' + '\n');
				return std::uint8_t(ch - 'a' + '\n');
	};

	std::vector<std::pair<std::int32_t, std::uint32_t>> offsets{};

	if (s == nullptr)
		s = &m_rawSectionHeaders[GetNumberOfSections() - 1];

	std::uint32_t start_offset = s->GetPointerToRawData();
	std::pair<std::int32_t, std::uint32_t> result{};
	std::uint32_t match_count = 0;

	for (std::uint32_t i = start_offset; i <= start_offset + s->GetSizeOfRawData(); ++i)
	{
		for (auto const& seq : binary_seq)
		{
			for (int c = 0; c < seq.second.size();)
			{
				if (seq.second[c] == ' ')
				{
					++c;
					continue;
				}

				if (seq.second[c] == '?')
				{
					++c;
					++match_count;
					continue;
				}

				std::uint8_t _byte = ((ascii_to_byte(seq.second[c]) << 4) | ascii_to_byte(seq.second[c + 1]));

				if (buffer()[i + match_count++] != _byte)
				{
					result = { 0,0 };
					break;
				}

				result = {seq.first, i};
				c += 2;
			}

			if (result.second)
			{
				offsets.emplace_back(std::move(result));
				break;
			}

			match_count = 0;
			result = { 0, 0 };
		}

		i += std::max<int>(match_count - 1, 0);
		match_count = 0;
	}

	return offsets;
}

template<unsigned int bitsize>
bool Image<bitsize>::AppendSection(std::string_view section_name, std::uint32_t size, std::uint32_t chrs, SectionHeader* out)
{
	std::uint32_t fileAlignment = GetPEHeader().GetOptionalHeader().GetFileAlignment();
	std::uint32_t sectAlignment = GetPEHeader().GetOptionalHeader().GetSectionAlignment();
	if (fileAlignment == 0 || sectAlignment == 0)
		return false;

	std::uint32_t alignedFileSize = Align(size, fileAlignment);
	std::uint32_t alignedVirtSize = Align(size, sectAlignment);

	//
	// Build a section (these should be the only necessary values to fill)
	SectionHeader sec;
	sec.SetName(section_name);
	sec.SetSizeOfRawData(alignedFileSize);
	sec.SetVirtualSize(alignedVirtSize);
	sec.SetCharacteristics(chrs);
	sec.SetVirtualAddress(GetPEHeader().GetNextSectionRva());
	sec.SetPointerToRawData(GetPEHeader().GetNextSectionOffset());

	//
	// Update image size
	GetPEHeader().GetOptionalHeader().SetSizeOfImage(GetPEHeader().GetOptionalHeader().GetSizeOfImage() + sec.GetVirtualSize());
	//
	// Update number of sections.
	GetPEHeader().GetFileHeader().SetNumberOfSections(GetNumberOfSections() + 1);

	//
	// Fill in some temp data
	std::vector<std::uint8_t> section_data(sec.GetSizeOfRawData());
	std::fill(section_data.begin(), section_data.end(), 0x0);

	//
	// Add it in the raw section header
	std::memcpy(&m_rawSectionHeaders[GetNumberOfSections() - 1], &sec, sizeof(SectionHeader));

	if (out)
		std::memcpy(out, &m_rawSectionHeaders[GetNumberOfSections() - 1], sizeof(SectionHeader));

	//
	// Finally, append it to the image buffer.
	buffer().insert_data(sec.GetPointerToRawData(), section_data.data(), section_data.size());

	//
	// Re-validate the image/headers.
	_validate();

	return true;
}


template<unsigned int bitsize>
void pepp::Image<bitsize>::SetMapped() noexcept
{
	for (std::uint16_t i = 0; i < GetNumberOfSections(); ++i)
	{
		SectionHeader& sec = GetSectionHeader(i);

		sec.SetPointerToRawData(sec.GetVirtualAddress());
		sec.SetSizeOfRawData(sec.GetVirtualSize());
	}

	m_mem_mapped = true;
}