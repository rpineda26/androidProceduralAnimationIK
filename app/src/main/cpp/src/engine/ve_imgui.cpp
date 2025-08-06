#include "ve_imgui.hpp"
#include "debug.hpp"
#include <ImGuizmo.h>
#include <iostream>
namespace ve{
    VkDescriptorPool VeImGui::createDescriptorPool(VkDevice device){
        VkDescriptorPool imGuiPool;
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        
        if(vkCreateDescriptorPool(device,
            &pool_info, nullptr, &imGuiPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
        return imGuiPool;
    }
    VkRenderPass VeImGui::createRenderPass(VkDevice device, VkFormat imageFormat, VkFormat depthFormat){
        
        VkRenderPass renderPass = VK_NULL_HANDLE;
        vkDestroyRenderPass(device, renderPass, nullptr);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};

        dependency.dstSubpass = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
        return renderPass;
    }

    void VeImGui::createImGuiContext( VeDevice& veDevice, VeWindow& veWindow, VkDescriptorPool imGuiPool, VkRenderPass renderPass, int imageCount){
        //imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        //Dear IMGUI style
        // ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        ImGui::StyleColorsClassic();
    
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        io.MouseDoubleClickTime = 0.3f;
        io.MouseDragThreshold = 15.0f;

        //setup renderer backend for imgui
        ImGui_ImplAndroid_Init(veWindow.getWindow());
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = veDevice.getInstance();
        init_info.PhysicalDevice = veDevice.getPhysicalDevice();
        init_info.Device = veDevice.device();
        init_info.QueueFamily = veDevice.graphicsQueueFamilyIndex();
        init_info.Queue = veDevice.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imGuiPool;
        init_info.Allocator = nullptr;
        init_info.MinImageCount = imageCount;
        init_info.ImageCount = imageCount;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;

        ImGui_ImplVulkan_Init(&init_info);
        ImFontConfig font_cfg;
        font_cfg.SizePixels = 32.0f;
        io.Fonts->AddFontDefault(&font_cfg);

        ImGuiStyle &style = ImGui::GetStyle();
        // Make scrollbars much thicker for mobile
        style.ScrollbarSize = 20.0f;  // Default is usually 14-16px
        style.ScrollbarRounding = 10.0f;
        style.GrabMinSize = 20.0f;  // Default is usually 10px
        style.GrabRounding = 10.0f;
        style.ScaleAllSizes(3.0f);
        float screen_width = 0.0f;
        float screen_height = 0.0f;

        screen_width = (float)ANativeWindow_getWidth(veWindow.getWindow());
        screen_height = (float)ANativeWindow_getHeight(veWindow.getWindow());
        io.DisplaySize = ImVec2(screen_width, screen_height);
        float aspect_ratio = screen_height / screen_width;
//        LOGI("Screen Width: %f", screen_width);
//        LOGI("Screen Height: %f", screen_height);
        // Scale fonts to maintain readability
        LOGI("native_width: %f, native_height: %f, io_width: %f, io_height: %f", screen_width, screen_height, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    }
    void VeImGui::initializeImGuiFrame(){
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

    }

    void VeImGui::renderImGuiFrame(VkCommandBuffer commandBuffer){
        ImGui::Render();
        ImDrawData* main_draw_data = ImGui::GetDrawData();

        if (main_draw_data) {
            ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);
        }
    }

    bool VeImGui::handleInput(const GameActivityMotionEvent* event) {
        ImGuiIO& io = ImGui::GetIO();

        static float lastX = 0.0f;
        static float lastY = 0.0f;
        static bool isDragging = false;

        // Store ImGuizmo state for your game logic to check
        static bool imguizmoActive = false;

        switch (event->action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
                lastX = event->pointers[0].rawX;
                lastY = event->pointers[0].rawY;
                isDragging = true;

                io.AddMousePosEvent(lastX, lastY);
                io.AddMouseButtonEvent(0, true);

                // Update ImGuizmo state
                imguizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

                return io.WantCaptureMouse || imguizmoActive;

            case AMOTION_EVENT_ACTION_UP:
                io.AddMousePosEvent(event->pointers[0].rawX, event->pointers[0].rawY);
                io.AddMouseButtonEvent(0, false);
                isDragging = false;
                imguizmoActive = false;

                return io.WantCaptureMouse || ImGuizmo::IsUsing();

            case AMOTION_EVENT_ACTION_MOVE:
                float currentX = event->pointers[0].rawX;
                float currentY = event->pointers[0].rawY;

                io.AddMousePosEvent(currentX, currentY);

                lastX = currentX;
                lastY = currentY;

                // Check ImGuizmo state during movement
                imguizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

                return io.WantCaptureMouse || imguizmoActive;
        }

        return false;
    }

    void VeImGui::cleanUpImGui(){
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();
    }
}