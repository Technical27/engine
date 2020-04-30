#ifndef ENGINE_IMAGE_HPP
#define ENGINE_IMAGE_HPP

#include <common.hpp>
#include <stb_image.h>
#include <vulkan.hpp>

#endif
void createTextureImage();
void createTextureImageView();
vk::ImageView createImageView(vk::Image image, vk::Format format);
