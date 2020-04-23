#include <main.hpp>
#include <vulkan.hpp>

VulkanContext vkctx;

int main() {
  bool quit = false;
  initSDL();
  initVulkan();
  SDL_Event* evt = new SDL_Event;
  while (!quit) {
    while (SDL_PollEvent(evt)) {
      switch (evt->type) {
      case SDL_QUIT:
      case SDL_KEYDOWN:
        quit = true;
        break;
      case SDL_WINDOWEVENT:
        switch (evt->window.event) {
        case SDL_WINDOWEVENT_RESIZED:
          vkctx.framebufferResized = true;
          break;
        case SDL_WINDOWEVENT_MINIMIZED:
          vkctx.minimized = true;
          break;
        case SDL_WINDOWEVENT_RESTORED:
          vkctx.minimized = false;
          break;
        }
        break;
      }
    }
    drawFrame();
  }
  delete evt;
  vkctx.device.waitIdle();
  cleanupVulkan();
  cleanupSDL();
}
