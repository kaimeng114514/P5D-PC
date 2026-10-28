// gxm_vulkan.h - minimal Vulkan backend for the P5D HLE (hand-written Vulkan ABI).
// Stage 4: integrate a working clear+present path into the AOT harness.
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <windows.h>

typedef uint32_t VkFlags;
typedef uint32_t VkBool32;
typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkDevice_T* VkDevice;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
typedef struct VkQueue_T* VkQueue;
typedef struct VkSwapchainKHR_T* VkSwapchainKHR;
typedef struct VkImage_T* VkImage;
typedef struct VkCommandPool_T* VkCommandPool;
typedef struct VkCommandBuffer_T* VkCommandBuffer;
typedef struct VkAllocationCallbacks VkAllocationCallbacks;
typedef enum VkResult { VK_SUCCESS = 0, VK_NOT_READY = 1, VK_INCOMPLETE = 5, VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    VK_ERROR_OUT_OF_DEVICE_MEMORY = -2, VK_ERROR_INITIALIZATION_FAILED = -3, VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    VK_ERROR_INCOMPATIBLE_DRIVER = -9, VK_SUBOPTIMAL_KHR = 1000001003 } VkResult;
typedef enum VkStructureType {
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 18, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 19,
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 20, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 12,
    VK_STRUCTURE_TYPE_SUBMIT_INFO = 4, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000,
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
} VkStructureType;
typedef enum VkQueueFlagBits { VK_QUEUE_GRAPHICS_BIT = 0x1, VK_QUEUE_COMPUTE_BIT = 0x2, VK_QUEUE_TRANSFER_BIT = 0x4 } VkQueueFlagBits;
typedef enum VkImageLayout { VK_IMAGE_LAYOUT_UNDEFINED = 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002 } VkImageLayout;
typedef enum VkPipelineStageFlagBits { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x1, VK_PIPELINE_STAGE_TRANSFER_BIT = 0x10, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x2000 } VkPipelineStageFlagBits;
typedef enum VkAccessFlagBits { VK_ACCESS_TRANSFER_WRITE_BIT = 0x100, VK_ACCESS_MEMORY_READ_BIT = 0x800 } VkAccessFlagBits;
typedef enum VkImageAspectFlagBits { VK_IMAGE_ASPECT_COLOR_BIT = 0x1 } VkImageAspectFlagBits;
typedef enum VkPresentModeKHR { VK_PRESENT_MODE_FIFO_KHR = 2 } VkPresentModeKHR;
typedef enum VkSurfaceTransformFlagBitsKHR { VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR = 0x1 } VkSurfaceTransformFlagBitsKHR;
typedef enum VkCompositeAlphaFlagBitsKHR { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 0x1 } VkCompositeAlphaFlagBitsKHR;
typedef enum VkSharingMode { VK_SHARING_MODE_EXCLUSIVE = 0 } VkSharingMode;
typedef enum VkImageUsageFlagBits { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x10, VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x2 } VkImageUsageFlagBits;
typedef enum VkCommandBufferLevel { VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0 } VkCommandBufferLevel;
typedef enum VkCommandBufferUsageFlagBits { VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x1 } VkCommandBufferUsageFlagBits;
typedef enum VkCommandPoolCreateFlagBits { VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 0x2 } VkCommandPoolCreateFlagBits;
typedef enum VkFormat { VK_FORMAT_UNDEFINED = 0, VK_FORMAT_B8G8R8A8_UNORM = 44 } VkFormat;
typedef enum VkColorSpaceKHR { VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0 } VkColorSpaceKHR;
#define VK_MAKE_VERSION(major, minor, patch) (((uint32_t)(major) << 22) | ((uint32_t)(minor) << 12) | (uint32_t)(patch))
#define VK_API_VERSION_1_0 VK_MAKE_VERSION(1, 0, 0)
typedef struct VkExtent2D { uint32_t width, height; } VkExtent2D;
typedef struct VkExtent3D { uint32_t width, height, depth; } VkExtent3D;
typedef struct VkApplicationInfo { VkStructureType sType; const void* pNext; const char* pApplicationName; uint32_t applicationVersion; const char* pEngineName; uint32_t engineVersion; uint32_t apiVersion; } VkApplicationInfo;
typedef struct VkInstanceCreateInfo { VkStructureType sType; const void* pNext; VkFlags flags; const VkApplicationInfo* pApplicationInfo; uint32_t enabledLayerCount; const char* const* ppEnabledLayerNames; uint32_t enabledExtensionCount; const char* const* ppEnabledExtensionNames; } VkInstanceCreateInfo;
typedef struct VkWin32SurfaceCreateInfoKHR { VkStructureType sType; const void* pNext; VkFlags flags; void* hinstance; void* hwnd; } VkWin32SurfaceCreateInfoKHR;
typedef struct VkQueueFamilyProperties { VkFlags queueFlags; uint32_t queueCount; uint32_t timestampValidBits; VkExtent3D minImageTransferGranularity; } VkQueueFamilyProperties;
typedef struct VkDeviceQueueCreateInfo { VkStructureType sType; const void* pNext; VkFlags flags; uint32_t queueFamilyIndex; uint32_t queueCount; const float* pQueuePriorities; } VkDeviceQueueCreateInfo;
typedef struct VkDeviceCreateInfo { VkStructureType sType; const void* pNext; VkFlags flags; uint32_t queueCreateInfoCount; const VkDeviceQueueCreateInfo* pQueueCreateInfos; uint32_t enabledLayerCount; const char* const* ppEnabledLayerNames; uint32_t enabledExtensionCount; const char* const* ppEnabledExtensionNames; const void* pEnabledFeatures; } VkDeviceCreateInfo;
typedef struct VkSurfaceCapabilitiesKHR { uint32_t minImageCount, maxImageCount; VkExtent2D currentExtent, minImageExtent, maxImageExtent; uint32_t maxImageArrayLayers; VkFlags supportedTransforms, currentTransform, supportedCompositeAlpha, supportedUsageFlags; } VkSurfaceCapabilitiesKHR;
typedef struct VkSurfaceFormatKHR { VkFormat format; VkColorSpaceKHR colorSpace; } VkSurfaceFormatKHR;
typedef struct VkSwapchainCreateInfoKHR {
    VkStructureType sType; const void* pNext; VkFlags flags;
    VkSurfaceKHR surface; uint32_t minImageCount; VkFormat imageFormat; VkColorSpaceKHR imageColorSpace;
    VkExtent2D imageExtent; uint32_t imageArrayLayers; VkFlags imageUsage;
    VkSharingMode imageSharingMode; uint32_t queueFamilyIndexCount; const uint32_t* pQueueFamilyIndices;
    VkSurfaceTransformFlagBitsKHR preTransform; VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkPresentModeKHR presentMode; VkBool32 clipped; VkSwapchainKHR oldSwapchain;
} VkSwapchainCreateInfoKHR;
typedef struct VkImageSubresourceRange { VkFlags aspectMask; uint32_t baseMipLevel, levelCount, baseArrayLayer, layerCount; } VkImageSubresourceRange;
typedef struct VkImageMemoryBarrier { VkStructureType sType; const void* pNext; VkFlags srcAccessMask, dstAccessMask;
    VkImageLayout oldLayout, newLayout; uint32_t srcQueueFamilyIndex, dstQueueFamilyIndex; VkImage image; VkImageSubresourceRange subresourceRange; } VkImageMemoryBarrier;
typedef union VkClearColorValue { float float32[4]; int32_t int32[4]; uint32_t uint32[4]; } VkClearColorValue;
typedef struct VkCommandPoolCreateInfo { VkStructureType sType; const void* pNext; VkFlags flags; uint32_t queueFamilyIndex; } VkCommandPoolCreateInfo;
typedef struct VkCommandBufferAllocateInfo { VkStructureType sType; const void* pNext; VkCommandPool commandPool; VkCommandBufferLevel level; uint32_t commandBufferCount; } VkCommandBufferAllocateInfo;
typedef struct VkCommandBufferBeginInfo { VkStructureType sType; const void* pNext; VkFlags flags; const void* pInheritanceInfo; } VkCommandBufferBeginInfo;
typedef struct VkSubmitInfo { VkStructureType sType; const void* pNext; uint32_t waitSemaphoreCount; const void* pWaitSemaphores; const VkFlags* pWaitDstStageMask;
    uint32_t commandBufferCount; const VkCommandBuffer* pCommandBuffers; uint32_t signalSemaphoreCount; const void* pSignalSemaphores; } VkSubmitInfo;
typedef struct VkPresentInfoKHR { VkStructureType sType; const void* pNext; uint32_t waitSemaphoreCount; const void* pWaitSemaphores;
    uint32_t swapchainCount; const VkSwapchainKHR* pSwapchains; const uint32_t* pImageIndices; VkResult* pResults; } VkPresentInfoKHR;

extern "C" {
VkResult vkCreateInstance(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
void vkDestroyInstance(VkInstance, const VkAllocationCallbacks*);
VkResult vkEnumeratePhysicalDevices(VkInstance, uint32_t*, VkPhysicalDevice*);
void* vkGetInstanceProcAddr(VkInstance, const char*);
void* vkGetDeviceProcAddr(VkDevice, const char*);
void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);
VkResult vkCreateDevice(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*);
void vkDestroyDevice(VkDevice, const VkAllocationCallbacks*);
void vkGetDeviceQueue(VkDevice, uint32_t, uint32_t, VkQueue*);
VkResult vkDeviceWaitIdle(VkDevice);
}

static HWND g_vkhwnd = nullptr;
static VkInstance g_vk_inst = nullptr;
static VkSurfaceKHR g_vk_surf = nullptr;
static VkDevice g_vk_dev = nullptr;
static VkQueue g_vk_queue = nullptr;
static VkSwapchainKHR g_vk_sc = nullptr;
static VkCommandPool g_vk_pool = nullptr;
static std::vector<VkImage> g_vk_imgs;
static VkExtent2D g_vk_ext{0, 0};
static int g_vk_ready = 0;
static uint32_t g_vk_frame = 0;
// function pointers
typedef VkResult (*PFN_CreateWin32Surface)(VkInstance, const VkWin32SurfaceCreateInfoKHR*, const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef VkResult (*PFN_GetSurfaceSupport)(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32*);
typedef VkResult (*PFN_GetSurfaceCaps)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR*);
typedef VkResult (*PFN_GetSurfaceFormats)(VkPhysicalDevice, VkSurfaceKHR, uint32_t*, VkSurfaceFormatKHR*);
typedef VkResult (*PFN_CreateSwapchain)(VkDevice, const VkSwapchainCreateInfoKHR*, const VkAllocationCallbacks*, VkSwapchainKHR*);
typedef VkResult (*PFN_GetSwapchainImages)(VkDevice, VkSwapchainKHR, uint32_t*, VkImage*);
typedef VkResult (*PFN_CreateCommandPool)(VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool*);
typedef VkResult (*PFN_AllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer*);
typedef VkResult (*PFN_BeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo*);
typedef VkResult (*PFN_EndCommandBuffer)(VkCommandBuffer);
typedef void (*PFN_CmdPipelineBarrier)(VkCommandBuffer, VkFlags, VkFlags, VkFlags, uint32_t, const void*, uint32_t, const void*, uint32_t, const VkImageMemoryBarrier*);
typedef void (*PFN_CmdClearColorImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue*, uint32_t, const VkImageSubresourceRange*);
typedef VkResult (*PFN_QueueSubmit)(VkQueue, uint32_t, const VkSubmitInfo*, void*);
typedef VkResult (*PFN_QueuePresent)(VkQueue, const VkPresentInfoKHR*);
typedef VkResult (*PFN_QueueWaitIdle)(VkQueue);

static PFN_CreateSwapchain P_CreateSwapchain;
static PFN_GetSwapchainImages P_GetSwapchainImages;
static PFN_CreateCommandPool P_CreateCommandPool;
static PFN_AllocateCommandBuffers P_AllocCmd;
static PFN_BeginCommandBuffer P_BeginCmd;
static PFN_EndCommandBuffer P_EndCmd;
static PFN_CmdPipelineBarrier P_Barrier;
static PFN_CmdClearColorImage P_ClearImage;
static PFN_QueueSubmit P_QueueSubmit;
static PFN_QueuePresent P_QueuePresent;
static PFN_QueueWaitIdle P_QueueWaitIdle;

static LRESULT CALLBACK gxmvk_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { return 0; }
    return DefWindowProc(h, m, w, l);
}

static int gxmvk_init() {
    if (g_vk_ready) return 0;
    WNDCLASSA wc = {};
    wc.lpfnWndProc = gxmvk_wndproc; wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "P5DHLEDisplay"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);
    g_vkhwnd = CreateWindowA("P5DHLEDisplay", "P5D (HLE Vulkan display)", WS_OVERLAPPEDWINDOW,
                             50, 50, 640, 480, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_vkhwnd) { printf("[GXMVK] CreateWindow FAILED\n"); return -1; }
    ShowWindow(g_vkhwnd, SW_SHOW); UpdateWindow(g_vkhwnd);

    VkApplicationInfo app{}; app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "P5D-HLE"; app.applicationVersion = 1; app.pEngineName = "P5D-AOT";
    app.engineVersion = 1; app.apiVersion = VK_API_VERSION_1_0;
    const char* exts[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    VkInstanceCreateInfo ci{}; ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = 2; ci.ppEnabledExtensionNames = exts;
    if (vkCreateInstance(&ci, nullptr, &g_vk_inst) != VK_SUCCESS) { printf("[GXMVK] instance FAILED\n"); return -1; }
    auto pCreateSurface = (PFN_CreateWin32Surface)vkGetInstanceProcAddr(g_vk_inst, "vkCreateWin32SurfaceKHR");
    auto pSurfaceSupport = (PFN_GetSurfaceSupport)vkGetInstanceProcAddr(g_vk_inst, "vkGetPhysicalDeviceSurfaceSupportKHR");
    auto pGetCaps = (PFN_GetSurfaceCaps)vkGetInstanceProcAddr(g_vk_inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    auto pGetFormats = (PFN_GetSurfaceFormats)vkGetInstanceProcAddr(g_vk_inst, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    if (!pCreateSurface || !pSurfaceSupport || !pGetCaps || !pGetFormats) { printf("[GXMVK] surface funcs missing\n"); return -1; }
    VkWin32SurfaceCreateInfoKHR sci{}; sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hinstance = wc.hInstance; sci.hwnd = g_vkhwnd;
    if (pCreateSurface(g_vk_inst, &sci, nullptr, &g_vk_surf) != VK_SUCCESS) { printf("[GXMVK] surface FAILED\n"); return -1; }
    uint32_t nd = 0; vkEnumeratePhysicalDevices(g_vk_inst, &nd, nullptr);
    if (!nd) { printf("[GXMVK] no device\n"); return -1; }
    std::vector<VkPhysicalDevice> devs(nd); vkEnumeratePhysicalDevices(g_vk_inst, &nd, devs.data());
    VkPhysicalDevice pd = devs[0];
    uint32_t nq = 0; vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, nullptr);
    std::vector<VkQueueFamilyProperties> qf(nq); vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf.data());
    int qidx = -1;
    for (uint32_t i = 0; i < nq; ++i) { VkBool32 present = 0; pSurfaceSupport(pd, i, g_vk_surf, &present); if ((qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) { qidx = (int)i; break; } }
    if (qidx < 0) { printf("[GXMVK] no present queue\n"); return -1; }
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{}; qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = (uint32_t)qidx; qci.queueCount = 1; qci.pQueuePriorities = &prio;
    const char* dexts[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo dci{}; dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1; dci.ppEnabledExtensionNames = dexts;
    if (vkCreateDevice(pd, &dci, nullptr, &g_vk_dev) != VK_SUCCESS) { printf("[GXMVK] device FAILED\n"); return -1; }
    vkGetDeviceQueue(g_vk_dev, (uint32_t)qidx, 0, &g_vk_queue);
    P_CreateSwapchain = (PFN_CreateSwapchain)vkGetDeviceProcAddr(g_vk_dev, "vkCreateSwapchainKHR");
    P_GetSwapchainImages = (PFN_GetSwapchainImages)vkGetDeviceProcAddr(g_vk_dev, "vkGetSwapchainImagesKHR");
    P_CreateCommandPool = (PFN_CreateCommandPool)vkGetDeviceProcAddr(g_vk_dev, "vkCreateCommandPool");
    P_AllocCmd = (PFN_AllocateCommandBuffers)vkGetDeviceProcAddr(g_vk_dev, "vkAllocateCommandBuffers");
    P_BeginCmd = (PFN_BeginCommandBuffer)vkGetDeviceProcAddr(g_vk_dev, "vkBeginCommandBuffer");
    P_EndCmd = (PFN_EndCommandBuffer)vkGetDeviceProcAddr(g_vk_dev, "vkEndCommandBuffer");
    P_Barrier = (PFN_CmdPipelineBarrier)vkGetDeviceProcAddr(g_vk_dev, "vkCmdPipelineBarrier");
    P_ClearImage = (PFN_CmdClearColorImage)vkGetDeviceProcAddr(g_vk_dev, "vkCmdClearColorImage");
    P_QueueSubmit = (PFN_QueueSubmit)vkGetDeviceProcAddr(g_vk_dev, "vkQueueSubmit");
    P_QueuePresent = (PFN_QueuePresent)vkGetDeviceProcAddr(g_vk_dev, "vkQueuePresentKHR");
    P_QueueWaitIdle = (PFN_QueueWaitIdle)vkGetDeviceProcAddr(g_vk_dev, "vkQueueWaitIdle");
    if (!P_CreateSwapchain || !P_GetSwapchainImages || !P_Barrier || !P_ClearImage || !P_QueuePresent) { printf("[GXMVK] device funcs missing\n"); return -1; }
    VkSurfaceCapabilitiesKHR caps{};
    pGetCaps(pd, g_vk_surf, &caps);
    uint32_t nf = 0; pGetFormats(pd, g_vk_surf, &nf, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(nf); pGetFormats(pd, g_vk_surf, &nf, formats.data());
    VkSurfaceFormatKHR fmt = formats.empty() ? VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } : formats[0];
    VkSwapchainCreateInfoKHR scc{}; scc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    scc.surface = g_vk_surf; scc.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && scc.minImageCount > caps.maxImageCount) scc.minImageCount = caps.maxImageCount;
    scc.imageFormat = fmt.format; scc.imageColorSpace = fmt.colorSpace; scc.imageExtent = caps.currentExtent;
    if (scc.imageExtent.width == 0xFFFFFFFFu) { scc.imageExtent.width = 640; scc.imageExtent.height = 480; }
    g_vk_ext = scc.imageExtent;
    scc.imageArrayLayers = 1; scc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    scc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; scc.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    scc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; scc.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    scc.clipped = 1;
    if (P_CreateSwapchain(g_vk_dev, &scc, nullptr, &g_vk_sc) != VK_SUCCESS) { printf("[GXMVK] swapchain FAILED\n"); return -1; }
    uint32_t nimg = 0; P_GetSwapchainImages(g_vk_dev, g_vk_sc, &nimg, nullptr);
    g_vk_imgs.resize(nimg); P_GetSwapchainImages(g_vk_dev, g_vk_sc, &nimg, g_vk_imgs.data());
    VkCommandPoolCreateInfo pci{}; pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; pci.queueFamilyIndex = (uint32_t)qidx;
    if (P_CreateCommandPool(g_vk_dev, &pci, nullptr, &g_vk_pool) != VK_SUCCESS) { printf("[GXMVK] pool FAILED\n"); return -1; }
    g_vk_ready = 1;
    printf("[GXMVK] backend ready: %ux%u, %u swapchain images, queue=%d\n", g_vk_ext.width, g_vk_ext.height, nimg, qidx);
    return 0;
}

static int gxmvk_present_clear(float rr, float gg, float bb) {
    if (!g_vk_ready) return -1;
    if (g_vk_imgs.empty()) return -1;
    uint32_t idx = g_vk_frame % (uint32_t)g_vk_imgs.size();
    VkCommandBufferAllocateInfo ai{}; ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = g_vk_pool; ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = 1;
    VkCommandBuffer cb = nullptr;
    if (P_AllocCmd(g_vk_dev, &ai, &cb) != VK_SUCCESS) return -1;
    VkCommandBufferBeginInfo bi{}; bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    P_BeginCmd(cb, &bi);
    VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkImageMemoryBarrier b1{}; b1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b1.srcAccessMask = 0; b1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b1.srcQueueFamilyIndex = 0xFFFFFFFFu; b1.dstQueueFamilyIndex = 0xFFFFFFFFu;
    b1.image = g_vk_imgs[idx]; b1.subresourceRange = range;
    P_Barrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b1);
    VkClearColorValue col{}; col.float32[0] = rr; col.float32[1] = gg; col.float32[2] = bb; col.float32[3] = 1.0f;
    P_ClearImage(cb, g_vk_imgs[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &col, 1, &range);
    VkImageMemoryBarrier b2{}; b2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    b2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    b2.srcQueueFamilyIndex = 0xFFFFFFFFu; b2.dstQueueFamilyIndex = 0xFFFFFFFFu;
    b2.image = g_vk_imgs[idx]; b2.subresourceRange = range;
    P_Barrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &b2);
    P_EndCmd(cb);
    VkSubmitInfo si{}; si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; si.commandBufferCount = 1; si.pCommandBuffers = &cb;
    if (P_QueueSubmit(g_vk_queue, 1, &si, nullptr) != VK_SUCCESS) return -1;
    VkPresentInfoKHR pi{}; pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.swapchainCount = 1; pi.pSwapchains = &g_vk_sc; pi.pImageIndices = &idx;
    VkResult pr = P_QueuePresent(g_vk_queue, &pi);
    if (P_QueueWaitIdle) P_QueueWaitIdle(g_vk_queue);
    ++g_vk_frame;
    { void* _bt[10]; USHORT _nb = CaptureStackBackTrace(0, 10, _bt, nullptr);
  printf("[GXMVK] present frame=%u image=%u result=%d base=%p bt[%u]:", g_vk_frame, idx, (int)pr, (void*)GetModuleHandleA(NULL), (unsigned)_nb);
  for (unsigned _i = 0; _i < _nb; ++_i) printf(" %p", _bt[_i]); printf("\n"); }
    return (pr == VK_SUCCESS || pr == VK_SUBOPTIMAL_KHR) ? 0 : -1;
}

// ---- GXM frame path helpers (stage 4) ----
static int g_vk_frame_count = 0;
static int gxmvk_begin_frame() { return (g_vk_ready ? 0 : -1); }
static int gxmvk_end_frame() {
    if (!g_vk_ready) return -1;
    int r = gxmvk_present_clear(0.10f, 0.35f, 0.80f);   // placeholder clear until real GXM draws are translated
    if (++g_vk_frame_count <= 10) printf("[GXMVK] end_frame #%d -> %d\n", g_vk_frame_count, r);
    return r;
}
static int gxmvk_present_now() { return gxmvk_end_frame(); }
