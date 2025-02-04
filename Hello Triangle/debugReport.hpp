#pragma once

#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Instance;

    class DebugReport
    {
    public:
        explicit DebugReport(const Instance & instance, vk::DebugReportFlagsEXT flags);
        DebugReport(const DebugReport &) = delete;
        DebugReport(DebugReport && other) = default;
        DebugReport & operator=(const DebugReport &) = delete;
        DebugReport & operator=(DebugReport &&) & = default;
        ~DebugReport() {}

        explicit operator const vk::UniqueDebugReportCallbackEXT &() const noexcept { return m_uniqueCallback; }

        enum class ReportLevel
        {
            Everything,
            WarningsAndAbove
        };
    private:
        vk::UniqueDebugReportCallbackEXT m_uniqueCallback;
    };

    static_assert(std::is_move_constructible_v<DebugReport>);
    static_assert(!std::is_copy_constructible_v<DebugReport>);
    static_assert(std::is_move_assignable_v<DebugReport>);
    static_assert(!std::is_copy_assignable_v<DebugReport>);
}
