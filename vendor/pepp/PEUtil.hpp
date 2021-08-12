#pragma once


namespace pepp
{
	//! Windows 32/64bit declare the page size as 4kb (4096)
	static constexpr std::uint32_t PAGE_SIZE = 0x1000;

	//! Align a value
	template<typename V_t>
	constexpr __forceinline auto Align(V_t value, std::uint32_t alignment) requires std::unsigned_integral<V_t>
	{
		if (alignment == 0)
			return value;
		return ((value + alignment) & ~(alignment - 1));
	}

	//! Make a value 4kb aligned (for section purposes)
	template<typename V_t>
	constexpr __forceinline auto Align4kb(V_t v) requires std::unsigned_integral<V_t>
	{
		return Align(v, PAGE_SIZE);
	}

	//! Demangle a mangled name (MS supplied)
	std::string DemangleName(std::string_view mangled_name);
}