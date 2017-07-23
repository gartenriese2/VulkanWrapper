#pragma once

#include "instance.hpp"

namespace bmvk
{
    class DebugReport
    {
    public:
        explicit DebugReport(const std::unique_ptr<Instance> & instancePtr, vk::DebugReportFlagsEXT flags);
        DebugReport(const DebugReport &) = delete;
        DebugReport(DebugReport && other) = default;
        DebugReport & operator=(const DebugReport &) = delete;
        DebugReport & operator=(DebugReport &&) & = default;
        ~DebugReport() {}
    private:
        vk::UniqueDebugReportCallbackEXT m_uniqueCallback;
    };

    static_assert(std::is_move_constructible_v<DebugReport>);
    static_assert(!std::is_copy_constructible_v<DebugReport>);
    static_assert(std::is_move_assignable_v<DebugReport>);
    static_assert(!std::is_copy_assignable_v<DebugReport>);
}