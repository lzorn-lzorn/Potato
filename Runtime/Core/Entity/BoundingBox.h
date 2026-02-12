
#pragma once
#include <type_traits>

namespace Core{

template <typename Ty, std::size_t Dimensions>
	requires std::is_arithmetic_v<Ty>
struct BoundingBox{

};

}