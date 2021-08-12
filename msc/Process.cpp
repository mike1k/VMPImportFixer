#include <Windows.h>
#include "Process.hpp"

using namespace vif::nt;

Process::Process(std::uint32_t processId) noexcept
	: m_processId(processId)
{
}

Process::Process(HANDLE processHandle) noexcept
	: m_handle(processHandle), m_processId(GetProcessId(processHandle))
{
}

bool Process::CreateHandle(std::uint32_t flags) noexcept
{
	m_handle = OpenProcess(flags, FALSE, m_processId);
	return m_handle != INVALID_HANDLE_VALUE;
}

bool Process::CreateHandle(std::uint32_t processId, std::uint32_t flags) noexcept
{
	m_processId = processId;
	m_handle = OpenProcess(flags, FALSE, m_processId);
	return m_handle != INVALID_HANDLE_VALUE;
}

// ReadProcessMemory will fail if the page has PAGE_NOACCESS or PAGE_GUARD, so try to change the protection and then read
static bool ReadPage(HANDLE handle, LPVOID address, LPVOID buffer, std::size_t size, SIZE_T* bytesRead)
{
	if (!ReadProcessMemory(handle, address, buffer, size, bytesRead))
	{
		DWORD oldProtect = 0;
		if (!VirtualProtectEx(handle, address, size, PAGE_READONLY, &oldProtect))
		{
			return false;
		}
		auto result = !!ReadProcessMemory(handle, address, buffer, size, bytesRead);
		VirtualProtectEx(handle, address, size, oldProtect, &oldProtect);
		return result;
	}
	return true;
}

bool Process::ReadMemory(pepp::Address<> address, void* buffer, std::size_t size) noexcept
{
	if (m_handle == INVALID_HANDLE_VALUE || !buffer || !size)
		return false;

	// Read page-by-page to maximize the likelyhook of reading successfully
	std::size_t bytesRead = 0;
	constexpr std::uintptr_t PAGE_SIZE = 0x1000;
	std::uintptr_t offset = 0;
	std::uintptr_t requestedSize = size;
	std::uintptr_t sizeLeftInFirstPage = PAGE_SIZE - (address.uintptr() & (PAGE_SIZE - 1));
	std::uintptr_t readSize = min(sizeLeftInFirstPage, requestedSize);

	while (readSize)
	{
		SIZE_T bytesReadSafe = 0;
		auto readSuccess = ReadPage(m_handle, (PVOID)(address.uintptr() + offset), (PBYTE)buffer + offset, readSize, &bytesReadSafe);
		bytesRead += bytesReadSafe;
		if (!readSuccess)
			break;

		offset += readSize;
		requestedSize -= readSize;
		readSize = min(PAGE_SIZE, requestedSize);
	}

	auto success = bytesRead == size;
	SetLastError(success ? ERROR_SUCCESS : ERROR_PARTIAL_COPY);
	return success;
}

bool Process::WriteMemory(pepp::Address<> address, void* buffer, std::size_t size) noexcept
{
	if (m_handle == INVALID_HANDLE_VALUE)
		return false;

	return static_cast<bool>(WriteProcessMemory(m_handle, address.as_ptr<void>(), buffer, size, nullptr));
}