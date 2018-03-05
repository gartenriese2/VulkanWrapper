#include <iostream>
#include <stdexcept>

#include "triangleDemo.hpp"
#include "vertexbufferDemo.hpp"
#include "stagingbufferDemo.hpp"
#include "indexbufferDemo.hpp"
#include "uniformbufferDemo.hpp"
#include "imguiDemo.hpp"
#include "textureDemo.hpp"
#include "combinedBufferDemo.hpp"
#include "depthBufferDemo.hpp"
#include "objectDemo.hpp"
#include "dragonDemo.hpp"
#include "coordinatesDemo.hpp"
#include "dynamicUboDemo.hpp"
#include "modelGroupDemo.hpp"
#include "pushConstantDemo.hpp"
#include "modelRepositoryDemo.hpp"

constexpr uint32_t k_width = 800;
constexpr uint32_t k_height = 600;

#ifdef NDEBUG
constexpr bool k_enableValidationLayers = false;
#else
constexpr bool k_enableValidationLayers = true;
#endif

template<typename T>
void runDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
{
    auto triangleDemoPtr{ std::make_unique<T>(enableValidationLayers, width, height) };
    triangleDemoPtr->run();
    triangleDemoPtr.reset(nullptr);
}

void runAllDemos(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
{
    runDemo<bmvk::TriangleDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::VertexbufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::StagingbufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::IndexbufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::UniformbufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::ImguiDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::TextureDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::CombinedBufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::DepthBufferDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::ObjectDemo<vw::scene::VertexDescription::NotUsed>>(enableValidationLayers, width, height);
    runDemo<bmvk::DragonDemo<vw::scene::VertexDescription::PositionNormalColorTexture>>(enableValidationLayers, width, height);
    runDemo<bmvk::CoordinatesDemo<vw::scene::VertexDescription::PositionNormalColor>>(enableValidationLayers, width, height);
    runDemo<bmvk::DynamicUboDemo<vw::scene::VertexDescription::PositionNormalColorTexture>>(enableValidationLayers, width, height);
    runDemo<bmvk::ModelGroupDemo<vw::scene::VertexDescription::PositionNormalColor>>(enableValidationLayers, width, height);
    runDemo<bmvk::PushConstantDemo<vw::scene::VertexDescription::PositionNormalColor>>(enableValidationLayers, width, height);
    runDemo<bmvk::ModelRepositoryDemo>(enableValidationLayers, width, height);
}

int main()
{
    try
    {
        //runAllDemos(k_enableValidationLayers, k_width, k_height);
        runDemo<bmvk::ModelRepositoryDemo>(k_enableValidationLayers, k_width, k_height);
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        std::cin.ignore();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}