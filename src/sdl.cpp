#include <SDL2/SDL_vulkan.h>
#include <common.hpp>

std::vector<const char*> getRequiredExtensions() {
  uint32_t extensionsCount;

  SDL_Vulkan_GetInstanceExtensions(vkctx.win, &extensionsCount, nullptr);
  std::vector<const char*> extensions(extensionsCount);
  SDL_Vulkan_GetInstanceExtensions(vkctx.win, &extensionsCount,
                                   extensions.data());

#ifdef USE_VALIDATION_LAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

void createSurface() {
  VkSurfaceKHR surf;
  SDL_Vulkan_CreateSurface(vkctx.win, vkctx.instance, &surf);
  vkctx.surface = surf;
}

void initSDL() {
  if (SDL_Init(SDL_INIT_EVENTS) != 0) {
    throw std::runtime_error("failed to init SDL");
  }

  vkctx.win = SDL_CreateWindow(
      "vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, vkctx.WIDTH,
      vkctx.HEIGHT,
      SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

  if (vkctx.win == nullptr) {
    throw std::runtime_error("failed to create window");
  }
}

void cleanupSDL() {
  SDL_DestroyWindow(vkctx.win);
  SDL_Quit();
}
