#pragma once

#include <memory>
#include <string>

namespace Runtime::Vulkan
{
    class Context
    {
    public:
        Context();
        ~Context();

        bool Initialize();
        void Shutdown();

        [[nodiscard]] std::string BackendName() const;
        [[nodiscard]] bool IsInitialized() const { return bInitialized; }

    private:
        bool bInitialized = false;
    };

    using ContextPtr = std::shared_ptr<Context>;
}
