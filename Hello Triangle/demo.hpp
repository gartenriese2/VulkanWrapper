#pragma once

#include <chrono>
#include <vulkan/vulkan.hpp>

#include <vw/camera.hpp>
#include <vw/window.hpp>
#include <vw/modelRepository.hpp>

#include "instance.hpp"
#include "device.hpp"
#include "queue.hpp"
#include "bufferFactory.hpp"

namespace bmvk
{
    template <vw::scene::VertexDescription VD>
    class Demo
    {
    public:
        Demo(const bool enableValidationLayers, const uint32_t width, const uint32_t height, std::string name, const DebugReport::ReportLevel reportLevel, const uint32_t maxModelRepositoryInstances = 1024);
        Demo(const Demo &) = delete;
        Demo(Demo && other) = default;
        Demo & operator=(const Demo &) = delete;
        Demo & operator=(Demo &&) = default;
        virtual ~Demo() {}

        virtual void run() {}

        virtual void keyCallback(int key, int scancode, int action, int mods) {}
    protected:
        vw::util::Camera m_camera;
        vw::util::Window m_window;
        Instance m_instance;
        Device m_device;
        Queue m_queue;
        vk::UniqueCommandPool m_commandPool;
        BufferFactory m_bufferFactory;

        vw::scene::ModelRepository<VD> m_modelRepository;

        double m_currentFrameTime = 0.0;
        double m_avgFrameTime = 0.0;
        double m_avgFps = 0.0;

        void copyBuffer(vk::UniqueBuffer & srcBuffer, vk::UniqueBuffer & dstBuffer, vk::DeviceSize size) const;
        void copyBufferToImage(vk::UniqueBuffer & buffer, vk::UniqueImage & image, uint32_t width, uint32_t height) const;
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory) const;
        void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage & image, vk::UniqueDeviceMemory & imageMemory);
        vk::UniqueImageView createImageView(const vk::UniqueImage & image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor) const;
        bool hasStencilComponent(const vk::Format format) const;
        void transitionImageLayout(const CommandBuffer & cmdBuffer, const vk::UniqueImage & image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
        void timing(const bool print = true);

    private:
        std::chrono::steady_clock::time_point m_timepoint;
        uint32_t m_timepointCount;
        std::chrono::microseconds m_elapsedTime;
    };
}
