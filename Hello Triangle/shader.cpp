#include "shader.hpp"

#include "device.hpp"
#include <vector>
#include <fstream>

namespace bmvk
{
    Shader::Shader(const std::experimental::filesystem::path & path, const Device & device)
        : m_code{ readFile(path) },
          m_module{ device.createShaderModule(m_code) }
    {
    }

    vk::PipelineShaderStageCreateInfo Shader::createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits flagBits) const
    {
        return { {}, flagBits, m_module.get(), "main" };
    }

    std::vector<char> Shader::readFile(const std::experimental::filesystem::path & path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}
