#include <iostream>
#include <stdexcept>

//#include "triangleDemo.hpp"
//#include "vertexbufferDemo.hpp"
//#include "stagingbufferDemo.hpp"
//#include "indexbufferDemo.hpp"
//#include "uniformbufferDemo.hpp"
//#include "imguiDemo.hpp"
//#include "textureDemo.hpp"
//#include "combinedBufferDemo.hpp"
//#include "depthBufferDemo.hpp"
//#include "objectDemo.hpp"
//#include "dragonDemo.hpp"
//#include "coordinatesDemo.hpp"
//#include "dynamicUboDemo.hpp"
#include "modelGroupDemo.hpp"
#include "pushConstantDemo.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

int main()
{
    try
    {
        //bmvk::TriangleDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::VertexbufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::StagingbufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::IndexbufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::UniformbufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::ImguiDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::TextureDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::CombinedBufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::DepthBufferDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::ObjectDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::DragonDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::CoordinatesDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::DynamicUboDemo app(enableValidationLayers, WIDTH, HEIGHT);
        bmvk::ModelGroupDemo app(enableValidationLayers, WIDTH, HEIGHT);
        //bmvk::PushConstantDemo app{ enableValidationLayers, WIDTH, HEIGHT };
        app.run();
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        std::cin.ignore();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}