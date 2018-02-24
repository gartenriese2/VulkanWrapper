#pragma once

#include <type_traits>
#include <vector>
#include <filesystem>
#include <vulkan/vulkan.hpp>

namespace bmvk
{
    class Device;

    class Shader
    {
    public:
        Shader(const std::experimental::filesystem::path & path, const Device & device);
        Shader(const Shader &) = delete;
        Shader(Shader && other) = default;
        Shader & operator=(const Shader &) = delete;
        Shader & operator=(Shader && other) = default;
        ~Shader() {}

        explicit operator const vk::UniqueShaderModule &() const noexcept { return m_module; }

        vk::PipelineShaderStageCreateInfo createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits flagBits) const;
    private:
        std::vector<char> m_code;
        vk::UniqueShaderModule m_module;

        static std::vector<char> Shader::readFile(const std::experimental::filesystem::path & path);
    };

    static_assert(std::is_move_constructible_v<Shader>);
    static_assert(std::is_move_assignable_v<Shader>);
    static_assert(!std::is_copy_constructible_v<Shader>);
    static_assert(!std::is_copy_assignable_v<Shader>);
}
