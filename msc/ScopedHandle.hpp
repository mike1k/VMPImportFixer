#pragma once

#include <pepp/misc/NonCopyable.hpp>

namespace vif::nt
{
	class ScopedHandle : pepp::msc::NonCopyable
	{
		HANDLE m_handle = INVALID_HANDLE_VALUE;

	public:
		ScopedHandle() = default;


		ScopedHandle(HANDLE handle)
			: m_handle(handle ? handle : INVALID_HANDLE_VALUE)
		{
		}

		~ScopedHandle()
		{
			if (m_handle != INVALID_HANDLE_VALUE)
				CloseHandle(m_handle);
		}

		operator HANDLE () noexcept
		{
			return m_handle;
		}

		HANDLE handle() noexcept
		{
			return m_handle;
		}

		LPHANDLE lphandle() noexcept
		{
			return &m_handle;
		}

		void operator=(HANDLE rhs) noexcept
		{
			if (m_handle != INVALID_HANDLE_VALUE)
				CloseHandle(m_handle);

			m_handle = rhs ? rhs : INVALID_HANDLE_VALUE;
		}
	};
}