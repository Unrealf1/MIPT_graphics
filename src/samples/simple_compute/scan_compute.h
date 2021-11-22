#ifndef SCAN_COMPUTE_H
#define SCAN_COMPUTE_H

#define VK_NO_PROTOTYPES
#include "../../render/compute_common.h"
#include "../resources/shaders/common.h"
#include <vk_descriptor_sets.h>
#include <vk_copy.h>

#include <string>
#include <iostream>
#include <memory>

class ScanCompute : public ICompute
{
    static constexpr int block_size = 256;
public:
    ScanCompute(uint32_t a_length);
    ~ScanCompute() { Cleanup(); };

    inline VkInstance   GetVkInstance() const override { return m_instance; }
    void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId) override;

    void Execute() override;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // debugging utils
    //
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        (void)flags;
        (void)objectType;
        (void)object;
        (void)location;
        (void)messageCode;
        (void)pUserData;
        std::cout << pLayerPrefix << ": " << pMessage << std::endl;
        return VK_FALSE;
    }

    VkDebugReportCallbackEXT m_debugReportCallback = nullptr;
private:

    VkInstance       m_instance = VK_NULL_HANDLE;
    VkCommandPool    m_commandPool = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device = VK_NULL_HANDLE;
    VkQueue          m_computeQueue = VK_NULL_HANDLE;
    VkQueue          m_transferQueue = VK_NULL_HANDLE;

    vk_utils::QueueFID_T m_queueFamilyIDXs{ UINT32_MAX, UINT32_MAX, UINT32_MAX };

    VkCommandBuffer m_cmdBufferCompute;
    VkCommandBuffer m_cmdBufferAggregate;
    VkFence m_fence;

    std::shared_ptr<vk_utils::DescriptorMaker> m_pBindings = nullptr;

    uint32_t m_length = 16u;
    uint32_t m_num_blocks = m_length / block_size + (m_length % block_size == 0 ? 0 : 1);

    VkPhysicalDeviceFeatures m_enabledDeviceFeatures = {};
    std::vector<const char*> m_deviceExtensions = {};
    std::vector<const char*> m_instanceExtensions = {};

    bool m_enableValidation;
    std::vector<const char*> m_validationLayers;
    std::shared_ptr<vk_utils::ICopyEngine> m_pCopyHelper;

    VkDescriptorSet       m_sumDS;
    VkDescriptorSetLayout m_sumDSLayout = nullptr;

    VkPipeline m_pipeline;
    VkPipeline m_pipeline_aggregate;
    VkPipelineLayout m_layout;
    VkPipelineLayout m_layout_aggregate;

    VkBuffer m_input, m_sums, m_output;

    void CreateInstance();
    void CreateDevice(uint32_t a_deviceId);

    void BuildCommandBufferSumTree(VkCommandBuffer a_cmdBuff, VkPipeline a_pipeline);
    void BuildCommandBufferAggregateTree(VkCommandBuffer a_cmdBuff, VkPipeline a_pipeline);

    void SetupPipelines();
    void CreateSumTreePipeline();
    void CreateAggregatePipeline();
    void CleanupPipeline();

    void Cleanup();

    void SetupValidationLayers();
};


#endif //SCAN_COMPUTE_H
