#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <functional>

template <typename T>
class VDeleter
{
public:
    VDeleter() : VDeleter([](T, VkAllocationCallbacks *) {}) {}

    VDeleter(std::function<void(T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks *)> deletef)
    {
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VDeleter()
    {
        cleanup();
    }

    const T * operator &() const
    {
        return &object;
    }

    T * replace()
    {
        cleanup();
        return &object;
    }

    operator T() const
    {
            return object;
    }

    void operator=(T rhs)
    {
        if (rhs != object)
        {
            cleanup();
            object = rhs;
        }
    }

    template<typename V>
    bool operator==(V rhs)
    {
        return object == T(rhs);
    }

private:

    T object{ VK_NULL_HANDLE };
    std::function<void(T)> deleter;

    void cleanup()
    {
        if (object != VK_NULL_HANDLE)
        {
            deleter(object);
        }

        object = VK_NULL_HANDLE;
    }
};

class HelloTriangleApplication
{
public:
    void run()
    {
        initVulkan();
        mainLoop();
    }

private:
    void initVulkan()
    {
        
    }

    void mainLoop()
    {
        
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}