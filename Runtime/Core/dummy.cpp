#include "Core.hpp"

#include <mutex>

namespace Runtime::Core
{
	namespace
	{
		std::mutex gCoreMutex;
		int gInitializationCounter = 0;
	}

	int gCoreVersion = 1;

	void InitializeCore()
	{
		std::scoped_lock lock(gCoreMutex);
		++gInitializationCounter;
	}

	void ShutdownCore()
	{
		std::scoped_lock lock(gCoreMutex);
		gInitializationCounter = 0;
	}

	int GetInitializationCount()
	{
		std::scoped_lock lock(gCoreMutex);
		return gInitializationCounter;
	}

	Transform::Transform() = default;

	void Transform::Translate(const Vector3D<float>& delta) noexcept
	{
		PositionVector += delta;
	}

	Vector3D<float> Transform::Position() const noexcept
	{
		return PositionVector;
	}
}