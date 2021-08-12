#pragma once

#include <fstream>

namespace pepp::io
{
    enum FileFlags {
        FILE_INPUT = 1,
        FILE_OUTPUT = 2,
        FILE_APP = 8,
        FILE_TRUNC = 16,
        FILE_BINARY = 32
    };

    class File {
    public:
        File() = default;
        File(const File& other) = default;

        File(std::string_view filename, int flags);
        File(File&& other);

        void Write(std::string_view text);
        void Write(const std::vector<std::uint8_t>& data);
        std::vector<std::uint8_t> Read();
        std::uintmax_t GetSize();

        File& operator=(File&& rhs);

    private:
        std::string   m_filename;
        int           m_flags;
        std::ofstream m_out_file;
        std::ifstream m_in_file;
    };
}