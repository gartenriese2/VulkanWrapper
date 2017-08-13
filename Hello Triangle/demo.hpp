#pragma once

#include <chrono>
#include <type_traits>

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"


namespace bmvk
{
    class Demo
    {
    public:
        Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name);
        Demo(const Demo &) = delete;
        Demo(Demo && other) = default;
        Demo & operator=(const Demo &) = delete;
        Demo & operator=(Demo && other) = default;
        virtual ~Demo() {}

        virtual void run() {}
    protected:
        Window m_window;
        Instance m_instance;
        Device m_device;

        std::chrono::steady_clock::time_point m_timepoint;
        uint32_t m_timepointCount;
        std::chrono::microseconds m_elapsedTime;

        void timing();
    };

    static_assert(std::is_move_constructible_v<Demo>);
    static_assert(std::is_move_assignable_v<Demo>);
    static_assert(!std::is_copy_constructible_v<Demo>);
    static_assert(!std::is_copy_assignable_v<Demo>);
}
