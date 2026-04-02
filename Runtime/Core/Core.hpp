#pragma once

#include "CoreAPI.hpp"
#include "Math/Vector.h"

namespace Runtime::Core
{
    
CORE_API void InitializeCore();
CORE_API void ShutdownCore();
CORE_API int GetInitializationCount();

CORE_API extern int gCoreVersion;

class CORE_API Transform
{
public:
    Transform();

    void Translate(const Math::Vector3D<float>& delta) noexcept;
    [[nodiscard]] Math::Vector3D<float> Position() const noexcept;

private:
    Math::Vector3D<float> PositionVector{};
};
}
