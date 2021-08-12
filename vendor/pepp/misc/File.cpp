#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include "File.hpp"

namespace pepp::io {

	File::File(std::string_view filename, int flags)
		: m_filename(filename)
		, m_flags(flags)
	{
	}

	File::File(File&& other)
		: m_filename(std::move(other.m_filename))
		, m_flags(other.m_flags)
	{
	}

	void File::Write(std::string_view text)
	{
		m_out_file.open(m_filename, m_flags & ~FILE_INPUT);
		if (m_out_file.is_open()) {
			m_out_file << text;
			m_out_file.close();
		}
	}

	void File::Write(const std::vector<std::uint8_t>& data)
	{
		m_out_file.open(m_filename, m_flags & ~FILE_INPUT);
		if (m_out_file.is_open()) {
			m_out_file.write((const char*)data.data(), data.size());
			m_out_file.close();
		}
	}

	std::vector<std::uint8_t> File::Read()
	{
		std::vector<std::uint8_t> file_buffer;

		m_in_file.open(m_filename, m_flags & ~FILE_OUTPUT);

		if (m_in_file.is_open()) {
			file_buffer = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(m_in_file), {});
			m_in_file.close();
		}

		return file_buffer;
	}

	std::uintmax_t File::GetSize()
	{
		return std::filesystem::file_size(m_filename);
	}

	File& io::File::operator=(File&& rhs)
	{
		if (this == &rhs)
			return *this;

		std::swap(m_filename, rhs.m_filename);
		std::swap(m_flags, rhs.m_flags);
		return *this;
	}

}


