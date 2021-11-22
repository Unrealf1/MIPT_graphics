#include "scan_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <numeric>

ScanCompute::ScanCompute(uint32_t a_length) : m_length(a_length)
{
#ifdef NDEBUG
    m_enableValidation = false;
#else
    m_enableValidation = true;
#endif
}

void ScanCompute::SetupValidationLayers()
{
    m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    m_validationLayers.push_back("VK_LAYER_LUNARG_monitor");
}

void ScanCompute::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
    m_instanceExtensions.clear();
    for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i) {
        m_instanceExtensions.push_back(a_instanceExtensions[i]);
    }
    SetupValidationLayers();
    VK_CHECK_RESULT(volkInitialize());
    CreateInstance();
    volkLoadInstance(m_instance);

    CreateDevice(a_deviceId);
    volkLoadDevice(m_device);

    m_commandPool = vk_utils::createCommandPool(m_device, m_queueFamilyIDXs.compute, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    m_cmdBufferCompute = vk_utils::createCommandBuffers(m_device, m_commandPool, 1)[0];
    m_cmdBufferAggregate = vk_utils::createCommandBuffers(m_device, m_commandPool, 1)[0];

    m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_physicalDevice, m_device, m_transferQueue, m_queueFamilyIDXs.compute, 8 * 1024 * 1024);
}


void ScanCompute::CreateInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "VkRender";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "ScanCompute";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

    m_instance = vk_utils::createInstance(m_enableValidation, m_validationLayers, m_instanceExtensions, &appInfo);
    if (m_enableValidation)
        vk_utils::initDebugReportCallback(m_instance, &debugReportCallbackFn, &m_debugReportCallback);
}

void ScanCompute::CreateDevice(uint32_t a_deviceId)
{
    m_physicalDevice = vk_utils::findPhysicalDevice(m_instance, true, a_deviceId, m_deviceExtensions);

    m_device = vk_utils::createLogicalDevice(m_physicalDevice, m_validationLayers, m_deviceExtensions,
        m_enabledDeviceFeatures, m_queueFamilyIDXs,
        VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

    vkGetDeviceQueue(m_device, m_queueFamilyIDXs.compute, 0, &m_computeQueue);
    vkGetDeviceQueue(m_device, m_queueFamilyIDXs.transfer, 0, &m_transferQueue);
}


void ScanCompute::SetupPipelines()
{
    std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             3}
    };

    // Создание и аллокация буферов
    m_input = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    m_output = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    m_sums = vk_utils::createBuffer(m_device, sizeof(float) * m_num_blocks, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    vk_utils::allocateAndBindWithPadding(m_device, m_physicalDevice, { m_input, m_output, m_sums }, 0);

    m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_device, dtypes, 1);

    // Создание descriptor set для передачи буферов в шейдер
    m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
    m_pBindings->BindBuffer(0, m_input);
    m_pBindings->BindBuffer(1, m_output);
    m_pBindings->BindBuffer(2, m_sums);
    m_pBindings->BindEnd(&m_sumDS, &m_sumDSLayout);

    // Заполнение буферов
    std::vector<float> values(m_length);
    for (uint32_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<float>(i);
    }
    m_pCopyHelper->UpdateBuffer(m_input, 0, values.data(), sizeof(float) * values.size());

    // should I put zeros in other buffers?
}

void ScanCompute::BuildCommandBufferSumTree(VkCommandBuffer a_cmdBuff, VkPipeline)
{
    vkResetCommandBuffer(a_cmdBuff, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Заполняем буфер команд
    VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, 1, &m_sumDS, 0, NULL);

    vkCmdPushConstants(a_cmdBuff, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);
    vkCmdDispatch(a_cmdBuff, m_num_blocks, 1, 1);


    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_aggregate);
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout_aggregate, 0, 1, &m_sumDS, 0, NULL);

    vkCmdPushConstants(a_cmdBuff, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);
    
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.buffer = m_sums;
    barrier.offset = 0;
    barrier.size = m_num_blocks;
    
    vkCmdPipelineBarrier(
        a_cmdBuff, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
        {}, 0, nullptr, 1, &barrier, 0, nullptr
    );

    vkCmdDispatch(a_cmdBuff, m_num_blocks, 1, 1);

    VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}

void ScanCompute::BuildCommandBufferAggregateTree(VkCommandBuffer a_cmdBuff, VkPipeline)
{
    vkResetCommandBuffer(a_cmdBuff, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Заполняем буфер команд
    VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, 1, &m_sumDS, 0, NULL);

    vkCmdPushConstants(a_cmdBuff, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);
    vkCmdDispatch(a_cmdBuff, m_num_blocks, 1, 1);

    VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}


void ScanCompute::CleanupPipeline()
{
    if (m_cmdBufferCompute)
    {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBufferCompute);
    }
    if (m_cmdBufferAggregate)
    {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBufferAggregate);
    }

    vkDestroyBuffer(m_device, m_input, nullptr);
    vkDestroyBuffer(m_device, m_output, nullptr);
    vkDestroyBuffer(m_device, m_sums, nullptr);

    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);

    vkDestroyPipelineLayout(m_device, m_layout_aggregate, nullptr);
    vkDestroyPipeline(m_device, m_pipeline_aggregate, nullptr);
}


void ScanCompute::Cleanup()
{
    CleanupPipeline();

    if (m_commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
}


void ScanCompute::CreateSumTreePipeline()
{
    // Загружаем шейдер
    std::vector<uint32_t> code = vk_utils::readSPVFile("../resources/shaders/sumtree.comp.spv");
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode = code.data();
    createInfo.codeSize = code.size() * sizeof(uint32_t);

    VkShaderModule shaderModule;
    // Создаём шейдер в вулкане
    VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo, NULL, &shaderModule));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";

    VkPushConstantRange pcRange = {};
    pcRange.offset = 0;
    pcRange.size = sizeof(m_length);
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Создаём layout для pipeline
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_sumDSLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_layout));

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = m_layout;

    // Создаём pipeline - объект, который выставляет шейдер и его параметры
    VK_CHECK_RESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &m_pipeline));

    vkDestroyShaderModule(m_device, shaderModule, nullptr);
}

void ScanCompute::CreateAggregatePipeline()
{
    // Загружаем шейдер
    std::vector<uint32_t> code = vk_utils::readSPVFile("../resources/shaders/aggregate_scan.comp.spv");
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode = code.data();
    createInfo.codeSize = code.size() * sizeof(uint32_t);

    VkShaderModule shaderModule;
    // Создаём шейдер в вулкане
    VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo, NULL, &shaderModule));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";

    VkPushConstantRange pcRange = {};
    pcRange.offset = 0;
    pcRange.size = sizeof(m_length);
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Создаём layout для pipeline
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_sumDSLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_layout_aggregate));

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = m_layout_aggregate;

    // Создаём pipeline - объект, который выставляет шейдер и его параметры
    VK_CHECK_RESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &m_pipeline_aggregate));

    vkDestroyShaderModule(m_device, shaderModule, nullptr);
}


void ScanCompute::Execute()
{
    SetupPipelines();
    CreateSumTreePipeline();
    CreateAggregatePipeline();

    BuildCommandBufferSumTree(m_cmdBufferCompute, nullptr);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmdBufferCompute;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, NULL, &m_fence));

    // Отправляем буфер команд на выполнение
    VK_CHECK_RESULT(vkQueueSubmit(m_computeQueue, 1, &submitInfo, m_fence));

    //Ждём конца выполнения команд
    VK_CHECK_RESULT(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, 100000000000));

    std::cout << "\nsums of blocks:\n";
    std::vector<float> values(m_num_blocks);
    m_pCopyHelper->ReadBuffer(m_sums, 0, values.data(), sizeof(float) * values.size());
    for (auto v : values) {
        std::cout << v << ' ';
    }
    std::vector<float> incremented(m_length);
    std::iota(incremented.begin(), incremented.end(), 0);
    std::vector<float> real_res(m_length);
    std::partial_sum(incremented.begin(), incremented.end(), real_res.begin());
    for (size_t i = real_res.size() - 1; i > 0; --i) {
        real_res[i] = real_res[i - 1];
    }
    std::cout << "\nres buffer:\n";
    std::vector<float> results(m_length);
    m_pCopyHelper->ReadBuffer(m_output, 0, results.data(), sizeof(float) * results.size());
    for (size_t i = 0; i < results.size(); ++i) {
        if ((std::abs(results[i] - real_res[i]) / real_res[i]) > 0.01f) {
            std::cout << i << ": " << results[i] << " != " << (real_res[i]) << "!!!\n";
            break;
        }
        if (i == results.size() - 1) {
            std::cout << "answer is correct!!!\n";
            
        }
    }
}
