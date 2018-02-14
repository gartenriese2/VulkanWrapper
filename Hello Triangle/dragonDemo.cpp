#include "dragonDemo.hpp"

#include <tinyobjloader/tiny_obj_loader.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "shader.hpp"
#include "bufferFactory.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vw/modelLoader.hpp>

namespace bmvk
{
    DragonDemo::DragonDemo(const bool enableValidationLayers, const uint32_t width, const uint32_t height)
        : ImguiBaseDemo{ enableValidationLayers, width, height, "Dragon Demo", true },
        m_imageAvailableSemaphore{ m_device.createSemaphore() },
        m_renderFinishedSemaphore{ m_device.createSemaphore() },
        m_renderImguiFinishedSemaphore{ m_device.createSemaphore() }
    {
        createDescriptorSetLayout();
        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        //loadModelWithAssimp();
        loadModel("../models/stanford_dragon/dragon.obj");
        createVertexBuffer(m_dragonModel);
        createIndexBuffer(m_dragonModel);
        //createCombinedBuffer();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createCommandBuffers();
    }

    void DragonDemo::run()
    {
        while (!m_window.shouldClose())
        {
            /*
            * CPU
            */

            m_window.pollEvents();

            updateUniformBuffer();
            imguiNewFrame();

            // 1. Show a simple window
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            {
                ImGui::SetNextWindowPos(ImVec2(20, 20));
                ImGui::SetNextWindowSize(ImVec2(400, 50), ImGuiCond_Always);
                ImGui::Begin("Performance");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", m_avgFrameTime / 1000.0, m_avgFps);
                ImGui::End();
            }

            ImGui::Render();

            /*
            * GPU
            */

            drawFrame();
        }

        m_device.waitIdle();
    }

    void DragonDemo::recreateSwapChain()
    {
        m_device.waitIdle();

        for (auto & fb : m_swapChainFramebuffers)
        {
            fb.reset(nullptr);
        }

        m_commandBuffers.clear();
        m_depthImageView.reset(nullptr);
        m_depthImageMemory.reset(nullptr);
        m_depthImage.reset(nullptr);
        m_graphicsPipeline.reset(nullptr);
        m_pipelineLayout.reset(nullptr);
        m_renderPass.reset(nullptr);

        ImguiBaseDemo::recreateSwapChain();

        createRenderPass();
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        createCommandBuffers();
    }

    void DragonDemo::createDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
        m_descriptorSetLayout = m_device.createDescriptorSetLayout({ uboLayoutBinding });
    }

    void DragonDemo::createRenderPass()
    {
        vk::AttachmentDescription colorAttachment{ {}, m_swapchain.getImageFormat().format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference colorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentDescription depthAttachment{ {}, m_instance.getPhysicalDevice().findDepthFormat(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::AttachmentReference depthAttachmentRef{ 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        vk::SubpassDescription subpass{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef, nullptr, &depthAttachmentRef };
        vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,{}, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite };
        std::vector<vk::AttachmentDescription> vec{ colorAttachment, depthAttachment };
        RenderPassCreateInfo renderPassInfo{ {}, vec, subpass, dependency };
        m_renderPass = static_cast<vk::Device>(m_device).createRenderPassUnique(renderPassInfo);
    }

    void DragonDemo::createGraphicsPipeline()
    {
        const Shader vertShader{ "../shaders/dragon.vert.spv", m_device };
        const Shader fragShader{ "../shaders/dragon.frag.spv", m_device };
        const auto vertShaderStageInfo{ vertShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex) };
        const auto fragShaderStageInfo{ fragShader.createPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment) };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = vw::util::Vertex::getBindingDescription();
        auto attributeDescriptions = vw::util::Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ PipelineVertexInputStateCreateInfo{ bindingDescription, attributeDescriptions } };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ {}, vk::PrimitiveTopology::eTriangleList };
        vk::Viewport viewport;
        vk::Rect2D scissor;
        const auto viewportState{ m_swapchain.getPipelineViewportStateCreateInfo(viewport, scissor) };
        vk::PipelineRasterizationStateCreateInfo rasterizer{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f };
        vk::PipelineMultisampleStateCreateInfo multisampling;
        vk::PipelineDepthStencilStateCreateInfo depthStencil{ {}, true, true, vk::CompareOp::eLess, false, false };
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{ false, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
        m_pipelineLayout = m_device.createPipelineLayout({ *m_descriptorSetLayout });

        vk::GraphicsPipelineCreateInfo pipelineInfo({}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, nullptr, *m_pipelineLayout, *m_renderPass, 0, nullptr, -1);
        m_graphicsPipeline = static_cast<vk::Device>(m_device).createGraphicsPipelineUnique(nullptr, pipelineInfo);
    }

    void DragonDemo::createFramebuffers()
    {
        m_swapChainFramebuffers.clear();
        for (auto & uniqueImageView : m_swapchain.getImageViews())
        {
            std::vector<vk::ImageView> imageViews{ *uniqueImageView, *m_depthImageView };
            m_swapChainFramebuffers.emplace_back(m_device.createFramebuffer(m_renderPass, imageViews, m_swapchain.getExtent().width, m_swapchain.getExtent().height, 1));
        }
    }

    void DragonDemo::createVertexBuffer(vw::util::Model & model) const
    {
        auto & modelVertices{ model.getVertices() };
        vk::DeviceSize bufferSize = sizeof(modelVertices[0]) * modelVertices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ static_cast<vk::Device>(m_device).mapMemory(*stagingBufferMemory, 0, bufferSize,{}) };
        memcpy(data, modelVertices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, model.getVertexBuffer(), model.getVertexBufferMemory());

        copyBuffer(stagingBuffer, model.getVertexBuffer(), bufferSize);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void DragonDemo::createIndexBuffer(vw::util::Model & model) const
    {
        auto & modelIndices{ model.getIndices() };
        vk::DeviceSize bufferSize = sizeof(modelIndices[0]) * modelIndices.size();

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        auto * data{ static_cast<vk::Device>(m_device).mapMemory(*stagingBufferMemory, 0, bufferSize,{}) };
        memcpy(data, modelIndices.data(), static_cast<size_t>(bufferSize));
        static_cast<vk::Device>(m_device).unmapMemory(*stagingBufferMemory);

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, model.getIndexBuffer(), model.getIndexBufferMemory());

        copyBuffer(stagingBuffer, model.getIndexBuffer(), bufferSize);

        stagingBuffer.reset(nullptr);
        stagingBufferMemory.reset(nullptr);
    }

    void DragonDemo::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer & buffer, vk::UniqueDeviceMemory & bufferMemory) const
    {
        vk::BufferCreateInfo bufferInfo{ {}, size, usage, vk::SharingMode::eExclusive };
        buffer = static_cast<vk::Device>(m_device).createBufferUnique(bufferInfo);

        const auto memRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*buffer) };
        vk::MemoryAllocateInfo allocInfo{ memRequirements.size, m_instance.getPhysicalDevice().findMemoryType(memRequirements.memoryTypeBits, properties) };
        bufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);

        static_cast<vk::Device>(m_device).bindBufferMemory(*buffer, *bufferMemory, 0);
    }

    void DragonDemo::loadModel(std::string_view file)
    {
        vw::util::ModelLoader ml;
        m_dragonModel = ml.loadModel(file);
        m_dragonModel.scale(glm::vec3{ 0.1f });

        //m_vertices.clear();
        //m_indices.clear();

        //Assimp::Importer importer;
        //const auto * scene = importer.ReadFile(file.data(), aiProcess_Triangulate);
        //if (!scene)
        //{
        //    throw std::runtime_error(importer.GetErrorString());
        //}

        //std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        //const auto meshes = scene->mMeshes;
        //const auto numMeshes = scene->mNumMeshes;
        //for (uint32_t i = 0; i < numMeshes; ++i)
        //{
        //    const auto mesh = meshes[i];
        //    const auto vertices = mesh->mVertices;
        //    const auto faces = mesh->mFaces;
        //    const auto numVertices = mesh->mNumVertices;
        //    const auto numFaces = mesh->mNumFaces;

        //    for (uint32_t j = 0; j < numFaces; ++j)
        //    {
        //        const auto face = faces[j];
        //        const auto indices = face.mIndices;
        //        const auto numIndices = face.mNumIndices;
        //        if (numIndices != 3)
        //        {
        //            throw std::runtime_error("no triangles");
        //        }

        //        const auto a_assimp = vertices[indices[0]];
        //        const auto a = glm::vec3(a_assimp.x, a_assimp.y, a_assimp.z);
        //        const auto b_assimp = vertices[indices[1]];
        //        const auto b = glm::vec3(b_assimp.x, b_assimp.y, b_assimp.z);
        //        const auto c_assimp = vertices[indices[2]];
        //        const auto c = glm::vec3(c_assimp.x, c_assimp.y, c_assimp.z);
        //        const auto n = glm::normalize(glm::cross(c - a, b - a));

        //        for (uint32_t k = 0; k < numIndices; ++k)
        //        {
        //            const auto index = indices[k];
        //            if (index >= numVertices)
        //            {
        //                throw std::runtime_error("index too big");
        //            }

        //            const auto v = vertices[index];

        //            Vertex vertex = {};

        //            vertex.pos = { v.x, v.y, v.z };
        //            //vertex.texCoord = { 0.f, 1.f };
        //            vertex.color = { 1.f, 0.f, 0.f };
        //            //vertex.normal = n;

        //            if (uniqueVertices.count(vertex) == 0)
        //            {
        //                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
        //                m_vertices.push_back(vertex);
        //            }

        //            m_indices.push_back(uniqueVertices[vertex]);
        //        }
        //    }
        //}
    }

    void DragonDemo::loadModelWithAssimp()
    {
        //Assimp::Importer importer;
        ////const auto * scene = importer.ReadFile("../models/stanford_dragon/dragon.obj", aiProcess_Triangulate);
        //const auto * scene = importer.ReadFile("../models/bunny/bun_zipper_res4.ply", aiProcess_Triangulate);
        //if (!scene) {
        //    throw std::runtime_error(importer.GetErrorString());
        //}

        //const auto meshes = scene->mMeshes;
        //const auto numMeshes = scene->mNumMeshes;
        //for (uint32_t i = 0; i < numMeshes; ++i)
        //{
        //    const auto mesh = meshes[i];
        //    const auto vertices = mesh->mVertices;
        //    const auto faces = mesh->mFaces;
        //    const auto numVertices = mesh->mNumVertices;
        //    const auto numFaces = mesh->mNumFaces;

        //    for (uint32_t j = 15; j < 20; ++j)
        //    {
        //        const auto face = faces[j];
        //        const auto indices = face.mIndices;
        //        const auto numIndices = face.mNumIndices;
        //        if (numIndices != 3)
        //        {
        //            throw std::runtime_error("no triangles");
        //        }

        //        for (uint32_t k = 0; k < numIndices; ++k)
        //        {
        //            const auto index = indices[k];
        //            if (index >= numVertices)
        //            {
        //                throw std::runtime_error("index too big");
        //            }

        //            m_indices.push_back(index);
        //        }
        //    }

        //    for (uint32_t j = 0; j < numVertices; ++j)
        //    {
        //        const auto vertex = vertices[j];
        //        Vertex v;
        //        v.color = { 1.f, 0.f, 0.f };
        //        v.pos = { vertex.x, vertex.y, vertex.z };
        //        m_vertices.push_back(v);
        //    }
        //}

        //for (uint32_t i = 0; i < m_indices.size(); i += 3)
        //{
        //    const auto x1{ m_vertices[m_indices[i]].pos.x };
        //    const auto y1{ m_vertices[m_indices[i]].pos.y };
        //    const auto z1{ m_vertices[m_indices[i]].pos.z };
        //    const auto x2{ m_vertices[m_indices[i + 1]].pos.x };
        //    const auto y2{ m_vertices[m_indices[i + 1]].pos.y };
        //    const auto z2{ m_vertices[m_indices[i + 1]].pos.z };
        //    const auto x3{ m_vertices[m_indices[i + 2]].pos.x };
        //    const auto y3{ m_vertices[m_indices[i + 2]].pos.y };
        //    const auto z3{ m_vertices[m_indices[i + 2]].pos.z };
        //    std::cout << "Face: A(" << x1 << "|" << y1 << "|" << z1 << "), B(" << x2 << "|" << y2 << "|" << z2 << "), C(" << x3 << "|" << y3 << "|" << z3 << ")\n";
        //}
    }

    void DragonDemo::loadModel()
    {
        /*tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "../models/stanford_dragon/dragon.obj", "../models/stanford_dragon/"))
        {
            throw std::runtime_error(err);
        }

        for (const auto & shape : shapes)
        {
            for (const auto & index : shape.mesh.indices)
            {
                Vertex vertex = {};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.color = { 1.0f, 0.0f, 0.0f };
                m_vertices.emplace_back(vertex);
                m_indices.push_back(static_cast<uint32_t>(m_indices.size()));
            }
        }*/

        // Loop over shapes
        //for (size_t s = 0; s < shapes.size(); s++)
        //{
        //    // Loop over faces(polygon)
        //    size_t index_offset = 0;
        //    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        //        int fv = shapes[s].mesh.num_face_vertices[f];

        //        // Loop over vertices in the face.
        //        for (size_t v = 0; v < fv; v++) {
        //            // access to vertex
        //            tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        //            tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
        //            tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
        //            tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
        //            /*tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
        //            tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
        //            tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
        //            tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
        //            tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];*/
        //            Vertex vertex = {};
        //            vertex.pos = { vx, vy, vz };
        //            vertex.color = { 1.f, 0.f, 0.f };
        //            m_vertices.push_back(vertex);
        //            m_indices.emplace_back(idx.vertex_index);
        //        }
        //        index_offset += fv;

        //        // per-face material
        //        shapes[s].mesh.material_ids[f];
        //    }
        //}

        //m_vertices.clear();
        //for (int i = 0; i < attrib.vertices.size(); i += 3)
        //{
        //    Vertex vertex = {};
        //    vertex.pos = { attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2] };
        //    vertex.color = { 1.f, 0.f, 0.f };
        //    m_vertices.push_back(vertex);
        //}

        //m_vertices.clear();
        //m_indices.clear();
        //std::ifstream file("../models/stanford_dragon/dragon.obj", std::ios::in);

        //if (!file.is_open())
        //{
        //    throw std::runtime_error("failed to open file!");
        //}

        //std::string line;
        //while (std::getline(file, line))
        //{
        //    if (line.size() > 1)
        //    {
        //        if (line[0] == 'v')
        //        {
        //            std::istringstream iss(line);
        //            char c;
        //            float x, y, z;
        //            if (!(iss >> c >> x >> y >> z)) { break; } // error
        //            Vertex vertex = {};
        //            vertex.pos = { x, y, z };
        //            vertex.color = { 1.f, 0.f, 0.f };
        //            m_vertices.push_back(vertex);
        //        }

        //        if (line[0] == 'f')
        //        {
        //            std::istringstream iss(line);
        //            char c;
        //            int i1, i2, i3;
        //            if (!(iss >> c >> i1 >> i2 >> i3)) { break; } // error
        //            m_indices.push_back(i1);
        //            m_indices.push_back(i2);
        //            m_indices.push_back(i3);
        //        }
        //    }
        //}
    }

    void DragonDemo::createDepthResources()
    {
        const auto depthFormat{ m_instance.getPhysicalDevice().findDepthFormat() };
        createImage(m_swapchain.getExtent().width, m_swapchain.getExtent().height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_depthImage, m_depthImageMemory);
        m_depthImageView = createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        transitionImageLayout(cmdBuffer, m_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();
    }

    void DragonDemo::createCombinedBuffer()
    {
        /*const auto vertexBufferSize{ sizeof m_vertices[0] * m_vertices.size() };
        const auto indexBufferSize{ sizeof m_indices[0] * m_indices.size() };

        auto vertexStagingBuffer{ m_bufferFactory.createStagingBuffer(vertexBufferSize) };
        vertexStagingBuffer.fill(m_vertices.data(), vertexBufferSize);
        auto indexStagingBuffer{ m_bufferFactory.createStagingBuffer(indexBufferSize) };
        indexStagingBuffer.fill(m_indices.data(), indexBufferSize);

        const auto vertexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer };
        const auto indexBufferUsageFlags{ vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer };
        const auto bufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eDeviceLocal };

        vk::BufferCreateInfo vertexBufferInfo{ {}, vertexBufferSize, vertexBufferUsageFlags };
        m_vertexBuffer = static_cast<vk::Device>(m_device).createBufferUnique(vertexBufferInfo);

        vk::BufferCreateInfo indexBufferInfo{ {}, indexBufferSize, indexBufferUsageFlags };
        m_indexBuffer = static_cast<vk::Device>(m_device).createBufferUnique(indexBufferInfo);

        const auto vertexMemRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_vertexBuffer) };
        const auto indexMemRequirements{ static_cast<vk::Device>(m_device).getBufferMemoryRequirements(*m_indexBuffer) };
        const auto vertexMemoryType{ m_instance.getPhysicalDevice().findMemoryType(vertexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };
        const auto indexMemoryType{ m_instance.getPhysicalDevice().findMemoryType(indexMemRequirements.memoryTypeBits, bufferMemoryPropertyFlags) };
        if (vertexMemoryType == indexMemoryType)
        {
            auto alignment = vertexMemRequirements.alignment;
            auto n = vertexBufferSize / alignment;
            auto m = vertexBufferSize % alignment;
            m_combinedBufferOffset = (n + m == 0 ? 0 : 1) * alignment;
            vk::MemoryAllocateInfo allocInfo{ vertexMemRequirements.size + indexMemRequirements.size, vertexMemoryType };
            m_combinedBufferMemory = static_cast<vk::Device>(m_device).allocateMemoryUnique(allocInfo);
        }
        else
        {
            throw std::runtime_error("memory can't be combined!");
        }

        static_cast<vk::Device>(m_device).bindBufferMemory(*m_vertexBuffer, *m_combinedBufferMemory, 0);
        static_cast<vk::Device>(m_device).bindBufferMemory(*m_indexBuffer, *m_combinedBufferMemory, m_combinedBufferOffset);

        auto cmdBuffer{ m_device.allocateCommandBuffer(m_commandPool) };
        cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        vertexStagingBuffer.buffer.copyToBuffer(cmdBuffer, m_vertexBuffer, vertexBufferSize);
        indexStagingBuffer.buffer.copyToBuffer(cmdBuffer, m_indexBuffer, indexBufferSize);
        cmdBuffer.end();
        m_queue.submit(cmdBuffer);
        m_queue.waitIdle();*/
    }

    void DragonDemo::createUniformBuffer()
    {
        const auto bufferSize{ sizeof(UniformBufferObject) };
        const auto uniformBufferUsageFlags{ vk::BufferUsageFlagBits::eUniformBuffer };
        const auto uniformBufferMemoryPropertyFlags{ vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };
        createBuffer(bufferSize, uniformBufferUsageFlags, uniformBufferMemoryPropertyFlags, m_uniformBuffer, m_uniformBufferMemory);
    }

    void DragonDemo::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
        std::vector<vk::DescriptorPoolSize> vec{ poolSize };
        m_descriptorPool = m_device.createDescriptorPool(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, vec);
    }

    void DragonDemo::createDescriptorSet()
    {
        vk::DescriptorSetLayout layouts[] = { *m_descriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocInfo{ *m_descriptorPool, 1, layouts };
        m_descriptorSets = static_cast<vk::Device>(m_device).allocateDescriptorSetsUnique(allocInfo);

        vk::DescriptorBufferInfo bufferInfo{ *m_uniformBuffer, 0, sizeof(UniformBufferObject) };
        WriteDescriptorSet descriptorWrite{ m_descriptorSets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo };
        std::vector<vk::WriteDescriptorSet> vec{ descriptorWrite };
        m_device.updateDescriptorSets(vec);
    }

    void DragonDemo::createCommandBuffers()
    {
        m_commandBuffers = m_device.allocateCommandBuffers(m_commandPool, static_cast<uint32_t>(m_swapChainFramebuffers.size()));
        for (size_t i = 0; i < m_commandBuffers.size(); ++i)
        {
            const auto & cmdBuffer{ m_commandBuffers[i] };
            cmdBuffer.begin(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            std::vector<vk::ClearValue> clearValues{ vk::ClearColorValue{ std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f } }, vk::ClearDepthStencilValue{ 1.f, 0 } };
            cmdBuffer.beginRenderPass(m_renderPass, m_swapChainFramebuffers[i], { { 0, 0 }, m_swapchain.getExtent() }, clearValues);
            cmdBuffer.bindPipeline(m_graphicsPipeline);
            cmdBuffer.bindDescriptorSet(m_pipelineLayout, m_descriptorSets[0]);
            cmdBuffer.bindVertexBuffer(m_dragonModel.getVertexBuffer());
            cmdBuffer.bindIndexBuffer(m_dragonModel.getIndexBuffer(), vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(static_cast<uint32_t>(m_dragonModel.getIndices().size()));
            cmdBuffer.endRenderPass();
            cmdBuffer.end();
        }
    }

    void DragonDemo::updateUniformBuffer()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.f;

        UniformBufferObject ubo;
        ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(5.f, 5.f, 5.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.f), m_swapchain.getExtent().width / static_cast<float>(m_swapchain.getExtent().height), 0.001f, 20.f);
        ubo.proj[1][1] *= -1;

        m_device.copyToMemory(m_uniformBufferMemory, ubo);
    }

    void DragonDemo::drawFrame()
    {
        m_queue.waitIdle();
        timing(false);

        uint32_t imageIndex;
        try
        {
            imageIndex = m_device.acquireNextImage(m_swapchain, m_imageAvailableSemaphore);
        }
        catch (const vk::OutOfDateKHRError &)
        {
            recreateSwapChain();
            return;
        }

        m_queue.submit(m_commandBuffers[imageIndex], m_imageAvailableSemaphore, m_renderFinishedSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
        ImguiBaseDemo::drawFrame(imageIndex, m_renderFinishedSemaphore, m_renderImguiFinishedSemaphore);

        auto waitSemaphore{ *m_renderImguiFinishedSemaphore };
        auto swapchain{ static_cast<vk::SwapchainKHR>(m_swapchain) };
        auto success{ m_queue.present(waitSemaphore, swapchain, imageIndex) };
        if (!success)
        {
            recreateSwapChain();
        }
    }
}
