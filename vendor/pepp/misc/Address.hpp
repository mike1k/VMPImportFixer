#pragma once


#include "Concept.hpp"

namespace pepp {
	template<typename ValueType = std::uintptr_t>
	class Address
	{
		ValueType	m_address;
	public:
		template<typename T>
		constexpr Address(T value = 0)  noexcept requires pepp::msc::MemoryAddress<T>
			: m_address((ValueType)(value))
		{
		}

		constexpr Address(const Address& rhs) = default;
		~Address() = default;

		Address& operator=(const Address& rhs) noexcept
		{
			m_address = rhs.m_address;
			return *this;
		}

		constexpr explicit operator std::uintptr_t() const noexcept
		{
			return m_address;
		}

		constexpr ValueType uintptr() const noexcept
		{
			return m_address;
		}

		template<typename C>
		C* as_ptr() noexcept requires pepp::msc::MemoryAddress<C*>
		{
			return reinterpret_cast<C*>(m_address);
		}

		template<typename C>
		C as() noexcept
		{
			return (C)m_address;
		}

		template<typename C>
		C& deref() noexcept
		{
			return *reinterpret_cast<C*>(m_address);
		}


		/*
		*! Comparison operators
		*/
		constexpr bool operator==(const Address& rhs) const noexcept
		{
			return m_address == rhs.m_address;
		}

		constexpr bool operator!=(const Address& rhs) const noexcept
		{
			return m_address != rhs.m_address;
		}

		constexpr bool operator>=(const Address& rhs) const noexcept
		{
			return m_address >= rhs.m_address;
		}

		constexpr bool operator<=(const Address& rhs) const noexcept
		{
			return m_address <= rhs.m_address;
		}

		constexpr bool operator>(const Address& rhs) const noexcept
		{
			return m_address > rhs.m_address;
		}

		constexpr bool operator<(const Address& rhs) const noexcept
		{
			return m_address < rhs.m_address;
		}

		/*
		/! Arithmetic operators
		*/


		constexpr Address operator+(const Address& rhs) const noexcept
		{
			return m_address + rhs.m_address;
		}

		constexpr Address operator-(const Address& rhs) const noexcept
		{
			return m_address - rhs.m_address;
		}

		constexpr Address operator*(const Address& rhs) const noexcept
		{
			return m_address * rhs.m_address;
		}

		constexpr Address operator/(const Address& rhs) const noexcept
		{
			return m_address / rhs.m_address;
		}

		/*
		/!
		*/

		constexpr Address& operator+=(const Address& rhs) noexcept
		{
			m_address += rhs.m_address;
			return *this;
		}

		constexpr Address& operator-=(const Address& rhs) noexcept
		{
			m_address -= rhs.m_address;
			return *this;
		}

		constexpr Address& operator*=(const Address& rhs) noexcept
		{
			m_address *= rhs.m_address;
			return *this;
		}


		/*
		/! Bitwise operators
		*/

		constexpr Address operator>>(const Address& rhs) const noexcept
		{
			return m_address >> rhs.m_address;
		}

		constexpr Address operator<<(const Address& rhs) const noexcept
		{
			return m_address << rhs.m_address;
		}

		constexpr Address operator^(const Address& rhs) const noexcept
		{
			return m_address ^ rhs.m_address;
		}

		constexpr Address operator&(const Address& rhs) const noexcept
		{
			return m_address & rhs.m_address;
		}

		constexpr Address operator|(const Address& rhs) const noexcept
		{
			return m_address | rhs.m_address;
		}

		/*
		/!
		*/

		constexpr Address& operator>>=(const Address& rhs) noexcept
		{
			m_address >>= rhs.m_address;
			return *this;
		}

		constexpr Address& operator<<=(const Address& rhs) noexcept
		{
			m_address <<= rhs.m_address;
			return *this;
		}

		constexpr Address& operator^=(const Address& rhs) noexcept
		{
			m_address ^= rhs.m_address;
			return *this;
		}

		constexpr Address& operator&=(const Address& rhs) noexcept
		{
			m_address &= rhs.m_address;
			return *this;
		}

		constexpr Address& operator|=(const Address& rhs) noexcept
		{
			m_address |= rhs.m_address;
			return *this;
		}
	};
}