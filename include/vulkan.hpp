#include <common.hpp>
#include <glm/glm.hpp>

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

  static vk::VertexInputBindingDescription getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex));

    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescription() {
    std::array<vk::VertexInputAttributeDescription, 2> descriptions;

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = vk::Format::eR32G32Sfloat;
    descriptions[0].offset = offsetof(Vertex, pos);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = vk::Format::eR32G32B32Sfloat;
    descriptions[1].offset = offsetof(Vertex, color);

    return descriptions;
  }
};

const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
#endif

void initVulkan();
void cleanupVulkan();
void drawFrame();
