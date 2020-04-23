#include <SDL2/SDL.h>
#include <bits/stdc++.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wtype-limits"
#include <vk_mem_alloc.h>
#pragma GCC diagnostic pop
#include <vulkan/vulkan.hpp>

#ifndef ENGINE_COMMON_HPP
#define ENGINE_COMMON_HPP

struct VulkanContext {
  const uint32_t HEIGHT = 600;
  const uint32_t WIDTH = 800;

  SDL_Window* win;

#ifdef USE_VALIDATION_LAYERS
  const std::vector<const char*> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  vk::DebugUtilsMessengerEXT debugMessenger;
#endif

  const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::Instance instance;
  vk::PhysicalDevice physicalDevice;
  vk::Device device;
  vk::SurfaceKHR surface;

  vk::SwapchainKHR swapchain;
  std::vector<vk::Image> swapchainImages;
  vk::Format swapchainImageFormat;
  vk::Extent2D swapchainExtent;
  std::vector<vk::ImageView> swapchainImageViews;

  vk::RenderPass renderPass;
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline pipeline;

  std::vector<vk::Framebuffer> framebuffers;

  vk::CommandPool commandPool;

  vk::Buffer vertexBuffer;
  VmaAllocation vertexAllocation;
  vk::Buffer indexBuffer;
  VmaAllocation indexAllocation;

  std::vector<vk::CommandBuffer> commandBuffers;

  vk::Queue graphicsQueue;
  vk::Queue presentQueue;

  std::vector<vk::Semaphore> imageSemaphores;
  std::vector<vk::Semaphore> renderSemaphores;
  std::vector<vk::Fence> inFlightFences;
  std::vector<std::optional<vk::Fence>> imagesInFlight;

  const uint8_t MAX_FRAMES_IN_FLIGHT = 2;
  uint8_t currentFrame = 0;

  bool framebufferResized = false;
  bool minimized = false;

  VmaAllocator allocator;
};

#endif

void initSDL();
void cleanupSDL();

std::vector<const char*> getRequiredExtensions();
void createSurface();

extern VulkanContext vkctx;
