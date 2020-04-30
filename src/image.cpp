#define STB_IMAGE_IMPLEMENTATION
#include <image.hpp>

vk::Image createImage(uint32_t width, uint32_t height, vk::Format format,
                      vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                      vk::MemoryPropertyFlags props,
                      VmaAllocation& allocation) {
  vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, format,
                                vk::Extent3D(static_cast<uint32_t>(width),
                                             static_cast<uint32_t>(height), 1),
                                1, 1, vk::SampleCountFlagBits::e1, tiling,
                                usage, vk::SharingMode::eExclusive);

  VmaAllocationCreateInfo imageAllocInfo = {};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  imageAllocInfo.requiredFlags = static_cast<uint32_t>(props);

  vk::Image output;

  vmaCreateImage(vkctx.allocator,
                 reinterpret_cast<VkImageCreateInfo*>(&imageInfo),
                 &imageAllocInfo, reinterpret_cast<VkImage*>(&output),
                 &allocation, nullptr);

  return output;
}

void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width,
                       uint32_t height) {
  vk::CommandBuffer commandBuffer = beginCommands();

  vk::BufferImageCopy region(
      0, 0, 0,
      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
      {0, 0, 0}, {width, height, 1});

  commandBuffer.copyBufferToImage(
      buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

  endCommands(commandBuffer);
}

void transitiionImageLayout(vk::Image image, vk::Format format,
                            vk::ImageLayout oldLayout,
                            vk::ImageLayout newLayout) {
  vk::CommandBuffer commandBuffer = beginCommands();

  vk::PipelineStageFlags srcStage, dstStage;

  vk::ImageMemoryBarrier barrier(
      static_cast<vk::AccessFlags>(0), static_cast<vk::AccessFlags>(0),
      oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
      image,
      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::invalid_argument("unsupported layout transition");
  }

  commandBuffer.pipelineBarrier(srcStage, dstStage,
                                static_cast<vk::DependencyFlags>(0), 0, nullptr,
                                0, nullptr, 1, &barrier);

  endCommands(commandBuffer);
}

void createTextureImage() {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load("textures/img.png", &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels)
    throw std::runtime_error("failed to load image");

  VmaAllocation stagingAllocation;
  vk::Buffer stagingBuffer =
      createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent,
                   VMA_MEMORY_USAGE_CPU_ONLY, stagingAllocation);

  VmaAllocationInfo allocInfo;
  vmaGetAllocationInfo(vkctx.allocator, stagingAllocation, &allocInfo);

  void* data = vkctx.device.mapMemory(allocInfo.deviceMemory, allocInfo.offset,
                                      imageSize);
  SDL_memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkctx.device.unmapMemory(allocInfo.deviceMemory);
  stbi_image_free(pixels);

  vkctx.textureImage = createImage(
      texWidth, texHeight, vk::Format::eR8G8B8A8Unorm,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
      vk::MemoryPropertyFlagBits::eDeviceLocal, vkctx.textureAllocation);
  transitiionImageLayout(vkctx.textureImage, vk::Format::eR8G8B8A8Unorm,
                         vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(stagingBuffer, vkctx.textureImage,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));
  transitiionImageLayout(vkctx.textureImage, vk::Format::eR8G8B8A8Unorm,
                         vk::ImageLayout::eTransferDstOptimal,
                         vk::ImageLayout::eShaderReadOnlyOptimal);
  vmaDestroyBuffer(vkctx.allocator, stagingBuffer, stagingAllocation);
}

vk::ImageView createImageView(vk::Image image, vk::Format format) {
  vk::ImageViewCreateInfo info(
      {}, image, vk::ImageViewType::e2D, format, {},
      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  return vkctx.device.createImageView(info);
}

void createTextureImageView() {
  vkctx.textureImageView =
      createImageView(vkctx.textureImage, vk::Format::eR8G8B8A8Unorm);
}
