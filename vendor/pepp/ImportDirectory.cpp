#include "PELibrary.hpp"

using namespace pepp;


// Explicit templates.
template class ImportDirectory<32>;
template class ImportDirectory<64>;

template<unsigned int bitsize>
bool ImportDirectory<bitsize>::ImportsModule(std::string_view module, std::uint32_t* name_rva) const
{
	auto descriptor = m_base;
	mem::ByteVector const* buffer = &m_image->buffer();

	while (descriptor->FirstThunk != 0) {
		std::uint32_t offset = m_image->GetPEHeader().RvaToOffset(descriptor->Name);

		std::string_view modname = buffer->as<const char*>(offset);

		if (_stricmp(modname.data(), module.data()) == 0)
		{
			if (name_rva)
				*name_rva = descriptor->Name;

			return true;
		}

		descriptor++;
	}

	if (name_rva)
		*name_rva = 0;

	return false;
}

template<unsigned int bitsize>
bool ImportDirectory<bitsize>::HasModuleImport(std::string_view module, std::string_view import, std::uint32_t* rva) const
{
	auto descriptor = m_base;
	mem::ByteVector const* buffer = &m_image->buffer();

	while (descriptor->Characteristics != 0) {
		std::uint32_t offset = m_image->GetPEHeader().RvaToOffset(descriptor->Name);

		if (_stricmp(buffer->as<const char*>(offset), module.data()) == 0)
		{
			std::int32_t index = 0;
			typename detail::Image_t<bitsize>::ThunkData_t* firstThunk =
				buffer->as<decltype(firstThunk)>(m_image->GetPEHeader().RvaToOffset(descriptor->OriginalFirstThunk));

			while (firstThunk->u1.AddressOfData)
			{
				//
				// TODO: Ordinals not handled here.
				if (IsImportOrdinal(firstThunk->u1.Ordinal))
				{
					index++;
					firstThunk++;
					continue;
				}

				IMAGE_IMPORT_BY_NAME* _imp =
					buffer->as<decltype(_imp)>(m_image->GetPEHeader().RvaToOffset(firstThunk->u1.AddressOfData));

				if (import == _imp->Name)
				{
					if (rva)
						*rva = descriptor->FirstThunk + (index * m_image->GetWordSize());

					return true;
				}

				index++;
				firstThunk++;
			}
		}

		descriptor++;
	}

	if (rva)
		*rva = 0;

	return false;
}

template<unsigned int bitsize>
void ImportDirectory<bitsize>::AddModuleImport(std::string_view module, std::string_view import, std::uint32_t* rva)
{
	// TODO: Clean this up and optimize some things.

	auto descriptor = m_base;
	mem::ByteVector* buffer = &m_image->buffer();

	std::unique_ptr<std::uint8_t> descriptors;
	std::uint32_t vsize = 0, rawsize = 0;

	vsize = m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).Size;


	descriptors.reset(new uint8_t[vsize]);
	memset(descriptors.get(), 0, vsize);

	SectionHeader newSec;

	//
	// Add in all the descriptors, so we can relocate them.
	while (descriptor->Characteristics != 0)
	{
		std::memcpy(&descriptors.get()[rawsize], descriptor, sizeof(*descriptor));
		rawsize += sizeof detail::Image_t<>::ImportDescriptor_t;

		std::memset(descriptor, 0x0, sizeof(*descriptor));

		descriptor++;
	}

	//
	// For the null term.
	rawsize += sizeof detail::Image_t<>::ImportDescriptor_t;

	//
	// Create a new section for the descriptors
	if (newSec = m_image->GetSectionHeader(".pepp"); newSec.GetName() == ".dummy")
	{
		//
		// We split a new section into two portions
		// The first part contains IAT addresses, or IMAGE_IMPORT_BY_NAME structs.
		// The second part contains import descriptors
		// NOTE: The section size may need to be modified depending on how many imports need to be added
		// This is using quite a large section due to a IAT rebuilding tool I created previously.
		m_image->AppendSection(
			".pepp",
			20 * PAGE_SIZE,
			SCN_MEM_READ |
			SCN_MEM_WRITE | 
			SCN_CNT_INITIALIZED_DATA |
			SCN_MEM_EXECUTE, &newSec);

		memset(buffer->as<void*>(newSec.GetPointerToRawData()), 0xcc, newSec.GetSizeOfRawData());

		newSec.SetPointerToRelocations(0);
		newSec.SetPointerToLinenumbers(0);
		newSec.SetNumberOfRelocations(0);
		newSec.SetNumberOfLinenumbers(0);

		// Ghetto, needed for now.
		memcpy(&m_image->GetSectionHeader(".pepp"), &newSec, sizeof newSec);

		//
		// Set the new base.
		m_base = reinterpret_cast<decltype(m_base)>(
			&m_image->base()[m_image->GetPEHeader().RvaToOffset(
				newSec.GetVirtualAddress() + (10*PAGE_SIZE))]);
	}

	//
	// Fill in the original descriptors
	std::memcpy(&buffer->at(newSec.GetPointerToRawData() + (10*PAGE_SIZE)), descriptors.get(), vsize);

	//
	// Set the new directory
	m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).VirtualAddress
		= newSec.GetVirtualAddress() + (10*PAGE_SIZE);
	m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).Size
		= vsize + sizeof detail::Image_t<>::ImportDescriptor_t;

	std::uint32_t descriptor_offset = newSec.GetPointerToRawData() + (10*PAGE_SIZE) + vsize - sizeof(*descriptor);
	descriptor = (decltype(descriptor)) & ((*buffer)[descriptor_offset]);

	//
	// Fill in default values, we don't use these
	descriptor->ForwarderChain = 0;
	descriptor->TimeDateStamp = 0;

	//
	// 1) Check if requested module already exists as string, and use that RVA
	std::uint32_t name_rva = 0;
	std::uint32_t tmp_offset = 0;
	std::uint32_t iat_rva = 0;
	std::uint32_t tmp_rva = 0;
	std::uint32_t oft_offset = 0;
	std::uint32_t oft_rva = 0;

	if (!ImportsModule(module, &name_rva))
	{
		// 2) If 1 isn't possible, add a section or extend the data section (hard)
		// and add in the module name manually
		// 	   - set descriptor->Name to that rva
		tmp_offset = m_image->FindPadding(&newSec, 0xcc, module.size() + 1);
		name_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

		std::memcpy(buffer->as<char*>(tmp_offset), module.data(), module.size());
		buffer->as<char*>(tmp_offset)[module.size()] = 0;
	}

	descriptor->Name = name_rva;

	using ImageThunkData_t = typename detail::Image_t<bitsize>::ThunkData_t;

	ImageThunkData_t thunks[2];

	// 3) Add in FirstThunk
	tmp_offset = m_image->FindPadding(&newSec, 0xcc, sizeof(thunks), m_image->GetWordSize());

	iat_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

	//
	// Fill in values so that it doesn't get taken up next time this function is called
	// Also, these need to be zero.
	memset(buffer->as<void*>(tmp_offset), 0x00, sizeof(thunks));

	ImageThunkData_t* firstThunk = m_image->buffer().as<ImageThunkData_t*>(tmp_offset);
	firstThunk->u1.AddressOfData = iat_rva;

	descriptor->FirstThunk = iat_rva;
	if (rva)
		*rva = iat_rva;

	// 4) Add in OriginalFirstThunk
	tmp_offset = m_image->FindPadding(&newSec, 0xcc, sizeof(thunks), m_image->GetWordSize());
	
	tmp_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

	//
	// Fill in values so that it doesn't get taken up next time this function is called
	// Also, these need to be zero.
	memset(buffer->as<void*>(tmp_offset), 0x00, sizeof(thunks));

	oft_offset = m_image->FindPadding(&newSec, 0xcc, sizeof(std::uint16_t) + import.size() + 1, m_image->GetWordSize());
	oft_rva = m_image->GetPEHeader().OffsetToRva(oft_offset);
	//
	// Copy in name to the oft rva
	IMAGE_IMPORT_BY_NAME* imp = buffer->as<IMAGE_IMPORT_BY_NAME*>(oft_offset);
	imp->Hint = 0x0000;
	
	memcpy(&imp->Name[0], import.data(), import.size());
	imp->Name[import.size()] = 0;

	ImageThunkData_t* ogFirstThunk = m_image->buffer().as<ImageThunkData_t*>(tmp_offset);
	ogFirstThunk->u1.AddressOfData = oft_rva;
	(ogFirstThunk + 1)->u1.AddressOfData = 0;

	descriptor->OriginalFirstThunk = tmp_rva;

	//
	// Finally null terminate
	memset((descriptor + 1), 0, sizeof(decltype(*descriptor)));
}

template<unsigned int bitsize>
void ImportDirectory<bitsize>::AddModuleImports(std::string_view module, std::initializer_list<std::string_view> imports, std::uint32_t* rva)
{
	// TODO: Clean this up and optimize some things.

	auto descriptor = m_base;
	mem::ByteVector* buffer = &m_image->buffer();

	std::unique_ptr<std::uint8_t> descriptors;
	std::uint32_t vsize = 0, rawsize = 0;

	vsize = m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).Size;


	descriptors.reset(new uint8_t[vsize]);
	memset(descriptors.get(), 0, vsize);

	SectionHeader newSec;

	//
	// Add in all the descriptors, so we can relocate them.
	while (descriptor->Characteristics != 0)
	{
		std::memcpy(&descriptors.get()[rawsize], descriptor, sizeof(*descriptor));
		rawsize += sizeof detail::Image_t<>::ImportDescriptor_t;

		std::memset(descriptor, 0x0, sizeof(*descriptor));

		descriptor++;
	}

	//
	// For the null term.
	rawsize += sizeof detail::Image_t<>::ImportDescriptor_t;

	//
	// Create a new section for the descriptors
	if (newSec = m_image->GetSectionHeader(".pepp"); newSec.GetName() == ".dummy")
	{
		//
		// We split a new section into two portions
		// The first part contains IAT addresses, or IMAGE_IMPORT_BY_NAME structs.
		// The second part contains import descriptors
		m_image->AppendSection(
			".pepp",
			2 * PAGE_SIZE,
			SCN_MEM_READ |
			SCN_MEM_WRITE |
			SCN_CNT_INITIALIZED_DATA |
			SCN_MEM_EXECUTE, &newSec);

		memset(buffer->as<void*>(newSec.GetPointerToRawData()), 0xcc, newSec.GetSizeOfRawData());

		newSec.SetPointerToRelocations(0);
		newSec.SetPointerToLinenumbers(0);
		newSec.SetNumberOfRelocations(0);
		newSec.SetNumberOfLinenumbers(0);

		// Ghetto, needed for now.
		memcpy(&m_image->GetSectionHeader(".pepp"), &newSec, sizeof newSec);

		//
		// Set the new base.
		m_base = reinterpret_cast<decltype(m_base)>(
			&m_image->base()[m_image->GetPEHeader().RvaToOffset(
				newSec.GetVirtualAddress() + PAGE_SIZE)]);
	}

	//
	// Fill in the original descriptors
	std::memcpy(&buffer->at(newSec.GetPointerToRawData() + PAGE_SIZE), descriptors.get(), vsize);

	//
	// Set the new directory
	m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).VirtualAddress
		= newSec.GetVirtualAddress() + PAGE_SIZE;
	m_image->GetPEHeader()
		.GetOptionalHeader()
		.GetDataDirectory(DIRECTORY_ENTRY_IMPORT).Size
		= vsize + sizeof detail::Image_t<>::ImportDescriptor_t;

	std::uint32_t descriptor_offset = newSec.GetPointerToRawData() + PAGE_SIZE + vsize - sizeof(*descriptor);
	descriptor = (decltype(descriptor)) & ((*buffer)[descriptor_offset]);

	//
	// Fill in default values, we don't use these
	descriptor->ForwarderChain = 0;
	descriptor->TimeDateStamp = 0;

	//
	// 1) Check if requested module already exists as string, and use that RVA
	std::uint32_t name_rva = 0;
	std::uint32_t tmp_offset = 0;
	std::uint32_t iat_rva = 0;
	std::uint32_t tmp_rva = 0;
	std::uint32_t oft_offset = 0;
	std::uint32_t oft_rva = 0;

	if (!ImportsModule(module, &name_rva))
	{
		// 2) If 1 isn't possible, add a section or extend the data section (hard)
		// and add in the module name manually
		// 	   - set descriptor->Name to that rva
		tmp_offset = m_image->FindPadding(&newSec, 0xcc, module.size() + 1);
		name_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

		std::memcpy(buffer->as<char*>(tmp_offset), module.data(), module.size());
		buffer->as<char*>(tmp_offset)[module.size()] = 0;
	}

	descriptor->Name = name_rva;

	using ImageThunkData_t = typename detail::Image_t<bitsize>::ThunkData_t;


	std::size_t thunksize = (imports.size() + 1) * sizeof(ImageThunkData_t);


	// 3) Add in FirstThunk
	tmp_offset = m_image->FindPadding(&newSec, 0xcc, thunksize, m_image->GetWordSize());
	iat_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

	//
	// Fill in values so that it doesn't get taken up next time this function is called
	// Also, these need to be zero.
	memset(buffer->as<void*>(tmp_offset), 0x00, thunksize);

	ImageThunkData_t* firstThunk = m_image->buffer().as<ImageThunkData_t*>(tmp_offset);
	firstThunk->u1.AddressOfData = iat_rva;

	descriptor->FirstThunk = iat_rva;
	


	// 4) Add in OriginalFirstThunk
	tmp_offset = m_image->FindPadding(&newSec, 0xcc, thunksize, m_image->GetWordSize());
	tmp_rva = m_image->GetPEHeader().OffsetToRva(tmp_offset);

	//
	// Fill in values so that it doesn't get taken up next time this function is called
	// Also, these need to be zero.
	memset(buffer->as<void*>(tmp_offset), 0x00, thunksize);


	ImageThunkData_t* ogFirstThunk = m_image->buffer().as<ImageThunkData_t*>(tmp_offset);

	int i = 0;
	for (auto it = imports.begin(); it != imports.end(); it++)
	{
		oft_offset = m_image->FindPadding(&newSec, 0xcc, sizeof(std::uint16_t) + it->size() + 1, m_image->GetWordSize());
		oft_rva = m_image->GetPEHeader().OffsetToRva(oft_offset);
		//
		// Copy in name to the oft rva
		IMAGE_IMPORT_BY_NAME* imp = buffer->as<IMAGE_IMPORT_BY_NAME*>(oft_offset);
		imp->Hint = 0x0000;

		memcpy(&imp->Name[0], it->data(), it->size());
		imp->Name[it->size()] = '\0';

		if (rva)
			rva[i] = iat_rva + (m_image->GetWordSize() * i++);

		ogFirstThunk->u1.AddressOfData = oft_rva;
		ogFirstThunk++;
	}

	
	ogFirstThunk->u1.AddressOfData = 0;
	descriptor->OriginalFirstThunk = tmp_rva;

	//
	// Finally null terminate
	memset((descriptor + 1), 0, sizeof(decltype(*descriptor)));
}

template<unsigned int bitsize>
void ImportDirectory<bitsize>::TraverseImports(const std::function<void(ModuleImportData_t*)>& cb_func)
{
	auto descriptor = m_base;
	mem::ByteVector const* buffer = &m_image->buffer();

	while (descriptor->Characteristics != 0) {
		std::uint32_t offset = m_image->GetPEHeader().RvaToOffset(descriptor->Name);
		const char* module = buffer->as<const char*>(offset);
		std::int32_t index = 0;
		typename detail::Image_t<bitsize>::ThunkData_t* firstThunk =
			buffer->as<decltype(firstThunk)>(m_image->GetPEHeader().RvaToOffset(descriptor->OriginalFirstThunk));

		ModuleImportData_t data{};
		data.module_name_rva = descriptor->Name;
		data.module_name = module;
		data.import_rva = -1;

		while (firstThunk->u1.AddressOfData)
		{
			IMAGE_IMPORT_BY_NAME* _imp =
				buffer->as<decltype(_imp)>(m_image->GetPEHeader().RvaToOffset(firstThunk->u1.AddressOfData));

			if (IsImportOrdinal(firstThunk->u1.Ordinal))
			{
				data.ordinal = true;
				data.import_variant = (std::uint64_t)firstThunk->u1.Ordinal;
				data.import_name_rva = 0;
			}
			else
			{
				data.import_variant = static_cast<char*>(_imp->Name);
				data.import_name_rva = firstThunk->u1.AddressOfData + sizeof(std::uint16_t);
			}

			data.import_rva = descriptor->FirstThunk + (index * m_image->GetWordSize());

			//
			// Call the callback
			cb_func(&data);

			index++;
			firstThunk++;
		}

		descriptor++;
	}
}

template<unsigned int bitsize>
void ImportDirectory<bitsize>::GetIATOffsets(std::uint32_t& begin, std::uint32_t& end) noexcept
{
	//
	// Null out.
	begin = end = 0;

	IMAGE_DATA_DIRECTORY const& iat = m_image->GetPEHeader().GetOptionalHeader().GetDataDirectory(IMAGE_DIRECTORY_ENTRY_IAT);
	if (iat.Size == 0)
		return;


	begin = m_image->GetPEHeader().RvaToOffset(iat.VirtualAddress);
	end = begin + iat.Size;
}
