#include <common.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <image.hpp>

#ifndef ENGINE_VULKAN_HPP
#define ENGINE_VULKAN_HPP
struct QueueIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  inline bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex));

    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 3>
  getAttributeDescription() {
    std::array<vk::VertexInputAttributeDescription, 3> descriptions;

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = vk::Format::eR32G32Sfloat;
    descriptions[0].offset = offsetof(Vertex, pos);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = vk::Format::eR32G32B32Sfloat;
    descriptions[1].offset = offsetof(Vertex, color);

    descriptions[2].binding = 0;
    descriptions[2].location = 2;
    descriptions[2].format = vk::Format::eR32G32Sfloat;
    descriptions[2].offset = offsetof(Vertex, texCoord);

    return descriptions;
  }
};

struct MVP {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
#endif
vk::Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                        vk::MemoryPropertyFlags props, VmaMemoryUsage memUsage,
                        VmaAllocation& allocation);

void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                vk::DeviceSize size);

void initVulkan();
void cleanupVulkan();
void drawFrame();
vk::CommandBuffer beginCommands();
void endCommands(vk::CommandBuffer& buffer);
