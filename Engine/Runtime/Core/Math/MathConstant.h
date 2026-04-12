
#ifndef CORE_MATHCONSTANT_H
#define CORE_MATHCONSTANT_H

namespace Core::Math
{

namespace Impl
{


static constexpr double epsilon   = std::numeric_limits<double>::epsilon();
static constexpr double infinity  = std::numeric_limits<double>::infinity();
static constexpr double pi        = 3.1415926535897932384626433832795028841971;
static constexpr double sqrt_pi   = 1.7724538509055160272981674833411451827975;  // sqrt(pi)
static constexpr double inv_pi    = 0.3183098861837906715377675267450287240689; // 1/pi
static constexpr double sqrt2     = 1.4142135623730950488016887242096980785697; // sqrt(2)
static constexpr double inv_sqrt2 = 0.7071067811865475244008443621048490392848; // 1/sqrt(2)



} // namespace Core::Math

#endif