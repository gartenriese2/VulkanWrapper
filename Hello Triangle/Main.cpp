#include <iostream>
#include <stdexcept>

//#include "triangleDemo.hpp"
//#include "vertexbufferDemo.hpp"
//#include "stagingbufferDemo.hpp"
//#include "indexbufferDemo.hpp"
//#include "uniformbufferDemo.hpp"
//#include "imguiDemo.hpp"
#include "textureDemo.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

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
        bmvk::TextureDemo app(enableValidationLayers, WIDTH, HEIGHT);
        app.run();
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}