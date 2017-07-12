#include <iostream>
#include <stdlib.h>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

int main()
{
    if (glfwInit() == GLFW_FALSE)
    {
        std::cout << "Failed to init glfw.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto * window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    const auto extensionProperties = vk::enumerateInstanceExtensionProperties();
    std::cout << "The following extensions are supported:\n";
    for (const auto & property : extensionProperties)
    {
        std::cout << '\t' << property.extensionName << '\n';
    }

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return EXIT_SUCCESS;
}