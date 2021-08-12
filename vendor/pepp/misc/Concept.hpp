#pragma once

#include <type_traits>

namespace pepp::msc
{
	template <class T>
	concept Arithmetic = std::is_arithmetic_v<T>;
	template <class T>
	concept MemoryAddress = std::is_integral_v<T> || std::is_pointer_v<T>;
}