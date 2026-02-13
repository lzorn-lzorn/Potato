
#pragma once
#include <type_traits>

#include "Math/Vector.h"

namespace Core{

template <typename Ty, std::size_t Dimensions>
	requires std::is_arithmetic_v<Ty>
struct BoundingBox{
	using value_type      = Ty;
	using reference       = Ty&;
	using pointer         = Ty*;
	using const_pointer   = const Ty*;
	using const_reference = const Ty&;

	static constexpr std::size_t dimensions = Dimensions;

	BoundingBox ()                   = default;
	BoundingBox (const BoundingBox&) = default;
	BoundingBox (BoundingBox&&)      = default;

	BoundingBox& operator= (const BoundingBox&) = default;
	BoundingBox& operator= (BoundingBox&&)      = default;
	virtual      ~BoundingBox()                 = default;

	virtual bool IsOverlap(const BoundingBox* other) const noexcept = 0;
};

}