#define VMA_IMPLEMENTATION
#include <vulkan.hpp>

#ifdef USE_VALIDATION_LAYERS
static inline VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {

  std::cerr << "layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

static inline void
populateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& info) {
  info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
  info.pfnUserCallback = debugCallback;
}
static vk::DebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(
    const vk::DebugUtilsMessengerCreateInfoEXT* pCreateInfo) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkctx.instance.getProcAddr(
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    VkDebugUtilsMessengerEXT msg;
    VkDebugUtilsMessengerCreateInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.flags = 0;
    info.messageSeverity = static_cast<uint32_t>(pCreateInfo->messageSeverity);
    info.messageType = static_cast<uint32_t>(pCreateInfo->messageType);
    info.pfnUserCallback = pCreateInfo->pfnUserCallback;
    func(static_cast<VkInstance>(vkctx.instance), &info, nullptr, &msg);
    return msg;
  } else
    throw std::runtime_error("can't find function");
}

static void
DestroyDebugUtilsMessengerEXT(vk::DebugUtilsMessengerEXT debugMessenger) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkctx.instance.getProcAddr(
      "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(vkctx.instance, debugMessenger, nullptr);
  else
    throw std::runtime_error("can't find function");
}

static inline void setupDebugMessenger() {
  vk::DebugUtilsMessengerCreateInfoEXT info;
  populateDebugCreateInfo(info);
  vkctx.debugMessenger = CreateDebugUtilsMessengerEXT(&info);
}

static bool checkValidationLayerSupport() {
  auto availableLayers = vk::enumerateInstanceLayerProperties();
  std::vector<std::string> availableLayerNames(availableLayers.size());

  std::transform(availableLayers.begin(), availableLayers.end(),
                 availableLayerNames.begin(),
                 [](auto x) { return std::string(x.layerName); });

  for (auto& layer : vkctx.validationLayers)
    if (std::find(availableLayerNames.begin(), availableLayerNames.end(),
                  std::string(layer)) == availableLayerNames.end())
      return false;

  return true;
}
#endif

static void createInstance() {
#ifdef USE_VALIDATION_LAYERS
  if (!checkValidationLayerSupport())
    throw std::runtime_error("instance dosen't support validation layers");
#endif

  vk::ApplicationInfo appinfo("vulkan", VK_MAKE_VERSION(1, 0, 0), nullptr,
                              VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_2);

  auto availableExtensions = vk::enumerateInstanceExtensionProperties();
  std::vector<std::string> availableExtensionNames(availableExtensions.size());

  std::transform(availableExtensions.begin(), availableExtensions.end(),
                 availableExtensionNames.begin(),
                 [](auto x) { return std::string(x.extensionName); });

  auto extensions = getRequiredExtensions();

  for (auto& ext : extensions) {
    if (std::find(availableExtensionNames.begin(),
                  availableExtensionNames.end(),
                  std::string(ext)) == availableExtensionNames.end())
      throw std::runtime_error(
          "instance does not support required extensions!");
  }

  vk::InstanceCreateInfo info(
      {}, &appinfo,
#ifdef USE_VALIDATION_LAYERS
      static_cast<uint32_t>(vkctx.validationLayers.size()),
      vkctx.validationLayers.data(),
#else
      0, nullptr,
#endif
      extensions.size(), extensions.data());
#ifdef USE_VALIDATION_LAYERS
  vk::DebugUtilsMessengerCreateInfoEXT debugInfo;
  populateDebugCreateInfo(debugInfo);
  info.pNext = &debugInfo;
#endif

  vkctx.instance = vk::createInstance(info);
}

static QueueIndices getQueueIndices(vk::PhysicalDevice device) {
  QueueIndices indices;

  auto queueFamiles = device.getQueueFamilyProperties();

  for (uint32_t i = 0; i < queueFamiles.size(); i++) {
    if (queueFamiles[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }
    if (device.getSurfaceSupportKHR(i, vkctx.surface)) {
      indices.presentFamily = i;
    }
  }

  return indices;
}

static bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
  auto availableExtensions = device.enumerateDeviceExtensionProperties();

  std::set<std::string> requiredExtensions(vkctx.deviceExtensions.begin(),
                                           vkctx.deviceExtensions.end());
  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(std::string(extension.extensionName));
  }

  return requiredExtensions.empty();
}

static inline SwapchainSupportDetails
querySwapchainSupport(vk::PhysicalDevice device) {
  SwapchainSupportDetails details;

  details.capabilities = device.getSurfaceCapabilitiesKHR(vkctx.surface);
  details.formats = device.getSurfaceFormatsKHR(vkctx.surface);
  details.presentModes = device.getSurfacePresentModesKHR(vkctx.surface);

  return details;
}

static inline bool checkDevice(vk::PhysicalDevice device) {
  auto indices = getQueueIndices(device);
  bool supported = indices.isComplete();

  if (!supported)
    return false;

  supported = checkDeviceExtensionSupport(device);

  if (!supported)
    return false;

  auto swapchainSupport = querySwapchainSupport(device);
  supported = !swapchainSupport.formats.empty() &&
              !swapchainSupport.presentModes.empty();

  return supported;
}

static void pickPhysicalDevice() {
  auto devices = vkctx.instance.enumeratePhysicalDevices();

  if (devices.empty())
    throw std::runtime_error("can't find any GPUs with vulkan support");

  bool deviceFound = false;

  for (const auto& device : devices) {
    if (checkDevice(device)) {
      vkctx.physicalDevice = device;
      deviceFound = true;
      break;
    }
  }

  if (!deviceFound)
    throw std::runtime_error("can't find suitable GPUs");
}

static void createDevice() {
  QueueIndices indices = getQueueIndices(vkctx.physicalDevice);

  const float priority = 1.0f;

  std::vector<vk::DeviceQueueCreateInfo> queueInfos;
  std::set<uint32_t> uniqueQueues = {indices.graphicsFamily.value(),
                                     indices.presentFamily.value()};

  for (uint32_t queueFamily : uniqueQueues) {
    queueInfos.push_back(
        vk::DeviceQueueCreateInfo({}, queueFamily, 1, &priority));
  }

  vk::PhysicalDeviceFeatures features;

  // if we use validation layers, then we enable them
  // otherwise we don't provide any layers
  // newer versions of vulkan ignore this only kept for compatibility purposes
  vk::DeviceCreateInfo info(
      {}, static_cast<uint32_t>(queueInfos.size()), queueInfos.data(),
#ifdef USE_VALIDATION_LAYERS
      static_cast<uint32_t>(vkctx.validationLayers.size()),
      vkctx.validationLayers.data(),
#else
      0, nullptr,
#endif
      static_cast<uint32_t>(vkctx.deviceExtensions.size()),
      vkctx.deviceExtensions.data(), &features);

  vkctx.device = vkctx.physicalDevice.createDevice(info);

  vkctx.graphicsQueue =
      vkctx.device.getQueue(indices.graphicsFamily.value(), 0);
  vkctx.presentQueue = vkctx.device.getQueue(indices.presentFamily.value(), 0);
}

static void createAllocator() {
  VmaAllocatorCreateInfo info = {};
  info.physicalDevice = vkctx.physicalDevice;
  info.device = vkctx.device;
  info.instance = vkctx.instance;

  vmaCreateAllocator(&info, &vkctx.allocator);
}

static vk::SurfaceFormatKHR
chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
  for (const auto& surfaceFormat : availableFormats) {
    if (surfaceFormat.format == vk::Format::eR8G8B8A8Srgb &&
        surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return surfaceFormat;
    }
  }
  return availableFormats[0];
}

static vk::PresentModeKHR choosePresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
  for (const auto& presentMode : availablePresentModes) {
    if (presentMode == vk::PresentModeKHR::eMailbox) {
      return presentMode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

static vk::Extent2D
chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    SDL_GetWindowSize(vkctx.win, &width, &height);
    vk::Extent2D extent = {static_cast<uint32_t>(width),
                           static_cast<uint32_t>(height)};

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);

    extent.height =
        std::clamp(extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return extent;
  }
}

static void createSwapchain() {
  auto swapchainSupport = querySwapchainSupport(vkctx.physicalDevice);

  auto surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
  auto presentMode = choosePresentMode(swapchainSupport.presentModes);
  auto extent = chooseExtent(swapchainSupport.capabilities);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR info(
      {}, vkctx.surface, imageCount, surfaceFormat.format,
      surfaceFormat.colorSpace, extent, 1,
      vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0,
      nullptr, swapchainSupport.capabilities.currentTransform,
      vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, VK_TRUE, nullptr);

  QueueIndices indices = getQueueIndices(vkctx.physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    info.setImageSharingMode(vk::SharingMode::eConcurrent);
    info.setQueueFamilyIndexCount(2);
    info.setPQueueFamilyIndices(queueFamilyIndices);
  }

  vkctx.swapchain = vkctx.device.createSwapchainKHR(info);
  vkctx.swapchainImages = vkctx.device.getSwapchainImagesKHR(vkctx.swapchain);
  vkctx.swapchainImageFormat = surfaceFormat.format;
  vkctx.swapchainExtent = extent;
}

static void createImageViews() {
  vkctx.swapchainImageViews.resize(vkctx.swapchainImages.size());
  for (size_t i = 0; i < vkctx.swapchainImages.size(); i++) {
    vk::ImageViewCreateInfo info(
        {}, vkctx.swapchainImages[i], vk::ImageViewType::e2D,
        vkctx.swapchainImageFormat, vk::ComponentMapping(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    vkctx.swapchainImageViews[i] = vkctx.device.createImageView(info);
  }
}

static std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("can't open file");
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}

static vk::ShaderModule createShaderModule(const std::vector<char>& code) {
  vk::ShaderModuleCreateInfo info(
      {}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));

  return vkctx.device.createShaderModule(info);
}

static void createRenderPass() {
  vk::AttachmentDescription colorAttachment(
      {}, vkctx.swapchainImageFormat, vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
      vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
      vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

  vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

  vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0,
                                 nullptr, 1, &colorRef);

  vk::SubpassDependency dependency(
      VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      static_cast<vk::AccessFlags>(0),
      vk::AccessFlagBits::eColorAttachmentWrite);

  vk::RenderPassCreateInfo info({}, 1, &colorAttachment, 1, &subpass, 1,
                                &dependency);

  vkctx.renderPass = vkctx.device.createRenderPass(info);
}

static void createPipeline() {
  auto vertCode = readFile("shaders/triangle.vert.spv");
  auto fragCode = readFile("shaders/triangle.frag.spv");

  auto vertModule = createShaderModule(vertCode);
  auto fragModule = createShaderModule(fragCode);

  vk::PipelineShaderStageCreateInfo vertShaderInfo(
      {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main");

  vk::PipelineShaderStageCreateInfo fragShaderInfo(
      {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main");

  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderInfo,
                                                      fragShaderInfo};
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescription();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
      {}, 1, &bindingDescription,
      static_cast<uint32_t>(attributeDescriptions.size()),
      attributeDescriptions.data());

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo(
      {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

  // we use dynamic states for viewport and scissor so we say that we have one
  // of each but specify as nullptr
  vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

  vk::PipelineRasterizationStateCreateInfo rasterizerInfo(
      {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
      vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE, 0.0f,
      0.0f, 0.0f, 1.0f);

  vk::PipelineMultisampleStateCreateInfo multisampleInfo(
      {}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE,
      VK_FALSE);

  vk::PipelineColorBlendAttachmentState colorBlendAttachment(VK_FALSE);
  colorBlendAttachment.setColorWriteMask(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

  vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
      {}, VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

  vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport,
                                      vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicInfo({}, 2, dynamicStates);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);

  vkctx.pipelineLayout = vkctx.device.createPipelineLayout(pipelineLayoutInfo);

  vk::GraphicsPipelineCreateInfo pipelineInfo(
      {}, 2, shaderStages, &vertexInputInfo, &inputAssemblyInfo, nullptr,
      &viewportState, &rasterizerInfo, &multisampleInfo, nullptr,
      &colorBlendInfo, &dynamicInfo, vkctx.pipelineLayout, vkctx.renderPass, 0);

  vkctx.pipeline = vkctx.device.createGraphicsPipeline(nullptr, pipelineInfo);

  vkctx.device.destroyShaderModule(vertModule);
  vkctx.device.destroyShaderModule(fragModule);
}

static void createFramebuffers() {
  vkctx.framebuffers.resize(vkctx.swapchainImageViews.size());
  for (size_t i = 0; i < vkctx.swapchainImageViews.size(); i++) {
    vk::ImageView attachments[] = {vkctx.swapchainImageViews[i]};

    vk::FramebufferCreateInfo info({}, vkctx.renderPass, 1, attachments,
                                   vkctx.swapchainExtent.width,
                                   vkctx.swapchainExtent.height, 1);

    vkctx.framebuffers[i] = vkctx.device.createFramebuffer(info);
  }
}

static void createCommandPool() {
  QueueIndices indices = getQueueIndices(vkctx.physicalDevice);

  vk::CommandPoolCreateInfo info({}, indices.graphicsFamily.value());
  vkctx.commandPool = vkctx.device.createCommandPool(info);
}

static vk::Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                               vk::MemoryPropertyFlags props,
                               VmaMemoryUsage memUsage,
                               VmaAllocation& allocation) {
  vk::BufferCreateInfo bufferInfo({}, size, usage);

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memUsage;
  allocInfo.requiredFlags = static_cast<uint32_t>(props);

  vk::Buffer output;

  vmaCreateBuffer(
      vkctx.allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo),
      &allocInfo, reinterpret_cast<VkBuffer*>(&output), &allocation, nullptr);

  return output;
}

static void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                       vk::DeviceSize size) {
  vk::CommandBufferAllocateInfo allocInfo(vkctx.commandPool,
                                          vk::CommandBufferLevel::ePrimary, 1);
  vk::CommandBuffer commandBuffer =
      vkctx.device.allocateCommandBuffers(allocInfo)[0];

  vk::CommandBufferBeginInfo beginInfo(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  commandBuffer.begin(beginInfo);
  vk::BufferCopy copy(0, 0, size);
  commandBuffer.copyBuffer(srcBuffer, dstBuffer, copy);
  commandBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkctx.graphicsQueue.submit(1, &submitInfo, nullptr);
  vkctx.graphicsQueue.waitIdle();

  vkctx.device.freeCommandBuffers(vkctx.commandPool, 1, &commandBuffer);
}

static void createVertexBuffer() {
  vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
  VmaAllocation stagingAllocation;
  auto stagingBuffer =
      createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent,
                   VMA_MEMORY_USAGE_CPU_ONLY, stagingAllocation);

  VmaAllocationInfo allocInfo;

  vmaGetAllocationInfo(vkctx.allocator, stagingAllocation, &allocInfo);

  void* data = vkctx.device.mapMemory(allocInfo.deviceMemory, allocInfo.offset,
                                      allocInfo.size);
  SDL_memcpy(data, vertices.data(), bufferSize);
  vkctx.device.unmapMemory(allocInfo.deviceMemory);

  vkctx.vertexBuffer =
      createBuffer(bufferSize,
                   vk::BufferUsageFlagBits::eTransferDst |
                       vk::BufferUsageFlagBits::eVertexBuffer,
                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                   VMA_MEMORY_USAGE_GPU_ONLY, vkctx.vertexAllocation);

  copyBuffer(stagingBuffer, vkctx.vertexBuffer, bufferSize);

  vmaDestroyBuffer(vkctx.allocator, stagingBuffer, stagingAllocation);
}

static void createIndexBuffer() {
  vk::DeviceSize bufferSize = sizeof(uint16_t) * indices.size();

  VmaAllocation stagingAllocation;
  auto stagingBuffer =
      createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent,
                   VMA_MEMORY_USAGE_CPU_ONLY, stagingAllocation);

  VmaAllocationInfo allocInfo;

  vmaGetAllocationInfo(vkctx.allocator, stagingAllocation, &allocInfo);

  void* data = vkctx.device.mapMemory(allocInfo.deviceMemory, allocInfo.offset,
                                      allocInfo.size);
  SDL_memcpy(data, indices.data(), bufferSize);
  vkctx.device.unmapMemory(allocInfo.deviceMemory);

  vkctx.indexBuffer =
      createBuffer(bufferSize,
                   vk::BufferUsageFlagBits::eTransferDst |
                       vk::BufferUsageFlagBits::eIndexBuffer,
                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                   VMA_MEMORY_USAGE_GPU_ONLY, vkctx.indexAllocation);

  copyBuffer(stagingBuffer, vkctx.indexBuffer, bufferSize);

  vmaDestroyBuffer(vkctx.allocator, stagingBuffer, stagingAllocation);
}

static void createCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo(
      vkctx.commandPool, vk::CommandBufferLevel::ePrimary,
      static_cast<uint32_t>(vkctx.framebuffers.size()));
  vkctx.commandBuffers = vkctx.device.allocateCommandBuffers(allocInfo);

  for (size_t i = 0; i < vkctx.commandBuffers.size(); i++) {
    vk::CommandBufferBeginInfo beginInfo;
    const auto& buffer = vkctx.commandBuffers[i];
    buffer.begin(beginInfo);

    vk::ClearValue clearValue(std::array<float, 4>({0.0f, 0.0f, 0.0f, 1.0f}));

    vk::RenderPassBeginInfo renderPassInfo(
        vkctx.renderPass, vkctx.framebuffers[i],
        vk::Rect2D({0, 0}, vkctx.swapchainExtent), 1, &clearValue);

    buffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkctx.pipeline);

    vk::Viewport viewport(
        0.0f, 0.0f, static_cast<float>(vkctx.swapchainExtent.width),
        static_cast<float>(vkctx.swapchainExtent.height), 0.0f, 1.0f);

    vk::Rect2D scissor({0, 0}, vkctx.swapchainExtent);

    buffer.setScissor(0, 1, &scissor);
    buffer.setViewport(0, 1, &viewport);
    buffer.bindVertexBuffers(0, std::array<vk::Buffer, 1>({vkctx.vertexBuffer}),
                             {0});
    buffer.bindIndexBuffer(vkctx.indexBuffer, 0, vk::IndexType::eUint16);
    buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    buffer.endRenderPass();

    buffer.end();
  }
}

static void createSyncObjects() {
  vk::SemaphoreCreateInfo sInfo;
  vk::FenceCreateInfo fInfo(vk::FenceCreateFlagBits::eSignaled);
  vkctx.imagesInFlight.resize(vkctx.swapchainImages.size());
  for (uint8_t i = 0; i < vkctx.MAX_FRAMES_IN_FLIGHT; i++) {
    vkctx.imageSemaphores.push_back(vkctx.device.createSemaphore(sInfo));
    vkctx.renderSemaphores.push_back(vkctx.device.createSemaphore(sInfo));
    vkctx.inFlightFences.push_back(vkctx.device.createFence(fInfo));
  }
}

static void cleanupSwapchain() {

  for (const auto& framebuffer : vkctx.framebuffers) {
    vkctx.device.destroyFramebuffer(framebuffer);
  }

  vkctx.device.freeCommandBuffers(vkctx.commandPool, vkctx.commandBuffers);
  vkctx.device.destroyRenderPass(vkctx.renderPass);

  for (const auto& imageView : vkctx.swapchainImageViews) {
    vkctx.device.destroyImageView(imageView);
  }

  vkctx.device.destroySwapchainKHR(vkctx.swapchain);
}

static void recreateSwapchain() {
  vkctx.device.waitIdle();

  cleanupSwapchain();

  createSwapchain();
  createImageViews();
  createRenderPass();
  createFramebuffers();
  createCommandBuffers();
}

void drawFrame() {
  if (vkctx.minimized)
    return;
  vkctx.device.waitForFences(1, &vkctx.inFlightFences[vkctx.currentFrame],
                             VK_TRUE, UINT64_MAX);

  auto [result, imageIndex] = vkctx.device.acquireNextImageKHR(
      vkctx.swapchain, UINT64_MAX, vkctx.imageSemaphores[vkctx.currentFrame],
      nullptr);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapchain();
    return;
  } else if (result != vk::Result::eSuccess &&
             result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swapchain image");
  }

  if (vkctx.imagesInFlight[vkctx.currentFrame].has_value()) {
    vkctx.device.waitForFences(
        1, &vkctx.imagesInFlight[vkctx.currentFrame].value(), VK_TRUE,
        UINT64_MAX);
  }

  vkctx.imagesInFlight[vkctx.currentFrame] =
      vkctx.inFlightFences[vkctx.currentFrame];

  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submitInfo(1, &vkctx.imageSemaphores[vkctx.currentFrame],
                            waitStages, 1, &vkctx.commandBuffers[imageIndex], 1,
                            &vkctx.renderSemaphores[vkctx.currentFrame]);

  vkctx.device.resetFences(vkctx.inFlightFences[vkctx.currentFrame]);

  vkctx.graphicsQueue.submit(submitInfo,
                             vkctx.inFlightFences[vkctx.currentFrame]);

  vk::PresentInfoKHR presentInfo(1, &vkctx.renderSemaphores[vkctx.currentFrame],
                                 1, &vkctx.swapchain, &imageIndex, nullptr);

  try {
    vk::Result res = vkctx.presentQueue.presentKHR(presentInfo);
    if (res == vk::Result::eSuboptimalKHR || vkctx.framebufferResized) {
      recreateSwapchain();
      vkctx.framebufferResized = false;
    }
  } catch (vk::OutOfDateKHRError) {
    recreateSwapchain();
    vkctx.framebufferResized = false;
  }
  vkctx.currentFrame = (vkctx.currentFrame + 1) % vkctx.MAX_FRAMES_IN_FLIGHT;
}

void initVulkan() {
  createInstance();
#ifdef USE_VALIDATION_LAYERS
  setupDebugMessenger();
#endif
  createSurface();
  pickPhysicalDevice();
  createDevice();
  createAllocator();
  createSwapchain();
  createImageViews();
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandPool();
  createVertexBuffer();
  createIndexBuffer();
  createCommandBuffers();
  createSyncObjects();
}

void cleanupVulkan() {
  cleanupSwapchain();
  vmaDestroyBuffer(vkctx.allocator, vkctx.vertexBuffer, vkctx.vertexAllocation);
  vmaDestroyBuffer(vkctx.allocator, vkctx.indexBuffer, vkctx.indexAllocation);
  vmaDestroyAllocator(vkctx.allocator);
  vkctx.device.destroyPipeline(vkctx.pipeline);
  vkctx.device.destroyPipelineLayout(vkctx.pipelineLayout);
  for (uint8_t i = 0; i < vkctx.MAX_FRAMES_IN_FLIGHT; i++) {
    vkctx.device.destroySemaphore(vkctx.imageSemaphores[i]);
    vkctx.device.destroySemaphore(vkctx.renderSemaphores[i]);
    vkctx.device.destroyFence(vkctx.inFlightFences[i]);
  }
  vkctx.device.destroyCommandPool(vkctx.commandPool);
  vkctx.instance.destroySurfaceKHR(vkctx.surface);
  vkctx.device.destroy();
#ifdef USE_VALIDATION_LAYERS
  DestroyDebugUtilsMessengerEXT(vkctx.debugMessenger);
#endif
  vkctx.instance.destroy();
}
