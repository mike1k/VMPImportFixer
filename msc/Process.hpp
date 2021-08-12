#pragma once

#include <pepp/misc/Address.hpp>
#include "ScopedHandle.hpp"
#include <pepp/misc/NonCopyable.hpp>

namespace vif::nt
{
	class Process : pepp::msc::NonCopyable
	{
	public:
		Process() = default;
		Process(std::uint32_t processId) noexcept;
		Process(HANDLE processHandle) noexcept;


		//! Create a handle to the process
		//! - returns true or false depending OpenProcess status
		bool CreateHandle(std::uint32_t flags) noexcept; 
		bool CreateHandle(std::uint32_t processId, std::uint32_t flags) noexcept;


		//! Read memory
		bool ReadMemory(pepp::Address<> address, void* buffer, std::size_t size) noexcept;

		//! Write memory
		bool WriteMemory(pepp::Address<> address, void* buffer, std::size_t size) noexcept;

		//! Get handle pointer
		HANDLE handle() noexcept { return m_handle.handle(); }

	private:
		std::uint16_t			m_processId = 0;
		vif::nt::ScopedHandle	m_handle;
	};
}