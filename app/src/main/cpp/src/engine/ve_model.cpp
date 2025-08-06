#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "ve_model.hpp"
#include "buffer.hpp"
#include "ve_swap_chain.hpp"
#include "utility.hpp"
#include "debug.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <vector>
#include <functional>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include <sstream>


namespace ve{
    struct VertexHash {
        size_t operator()(const VeModel::Vertex& vertex) const {
            size_t hash = 0;

            // Hash position components
            hash ^= std::hash<float>{}(vertex.position.x) + 0x9e3779b9;
            hash ^= std::hash<float>{}(vertex.position.y) << 1;
            hash ^= std::hash<float>{}(vertex.position.z) << 2;

            // Hash color components
            hash ^= std::hash<float>{}(vertex.color.r) + 0x9e3779b9;
            hash ^= std::hash<float>{}(vertex.color.g) << 3;
            hash ^= std::hash<float>{}(vertex.color.b) << 4;

            // Hash normal
            hash ^= std::hash<float>{}(vertex.normal.x) + 0x9e3779b9;
            hash ^= std::hash<float>{}(vertex.normal.y) << 5;
            hash ^= std::hash<float>{}(vertex.normal.z) << 6;

            // Hash UV
            hash ^= std::hash<float>{}(vertex.uv.x) + 0x9e3779b9;
            hash ^= std::hash<float>{}(vertex.uv.y) << 7;

            return hash;
        }
    };
    VeModel::VeModel(VeDevice& device, const VeModel::Builder &builder): veDevice(device){
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }
    //buffer cleanup handled by Buffer class
    VeModel::~VeModel(){}
    void VeModel::createVertexBuffers(const std::vector<Vertex>& vertices){
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        //create staging buffer
        uint32_t vertexSize = sizeof(vertices[0]);
        VeBuffer stagingBuffer{veDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)vertices.data());

        vertexBuffer = std::make_unique<VeBuffer>(veDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        veDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }
    void VeModel::createIndexBuffers(const std::vector<uint32_t>& indices){
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer =  indexCount > 0;
        //index buffer is optional
        if(!hasIndexBuffer){
            return;
        }
        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);
        //create staging buffer
        VeBuffer stagingBuffer{veDevice, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *)indices.data());
        //create index buffer
        indexBuffer = std::make_unique<VeBuffer>(veDevice, indexSize, indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        veDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void VeModel::bind(VkCommandBuffer commandBuffer){
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        if(hasIndexBuffer){
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }
    void VeModel::draw(VkCommandBuffer commandBuffer){
        if(hasIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }else{
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }
    void VeModel::drawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount){
        if(hasIndexBuffer){
            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
        }else{
            vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
        }
    }
    void VeModel::updateAnimation(float deltaTime, int frameCounter, int frameIndex){
        if(hasAnimation){
            animationManager->update(deltaTime, *skeleton, frameCounter);
            skeleton->update();


        }
    }
    std::vector<VkVertexInputBindingDescription> VeModel::Vertex::getBindingDescriptions(){
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }
    std::vector<VkVertexInputAttributeDescription> VeModel::Vertex::getAttributeDescriptions(){
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
        attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)});
        attributeDescriptions.push_back({5, 0, VK_FORMAT_R32G32B32A32_SINT, offsetof(Vertex, jointIndices)});
        attributeDescriptions.push_back({6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, jointWeights)});
        return attributeDescriptions;
    }
    
    std::unique_ptr<VeModel> VeModel::createModelFromFile(VeDevice& device, AAssetManager *assetManager, VeDescriptorPool& descriptorPool, const std::string& filePath){
        Builder builder{};
        std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
        if (extension == "gltf" || extension == "glb") {
            builder.loadModelGLTF(filePath, assetManager);
        }
        
        auto model = std::make_unique<VeModel>(device, builder);
        if(extension == "gltf" || extension == "glb"){
            model->loadSkeleton(builder.model);
            model->loadAnimations(builder.model);
        }
        MaterialComponent mat{};
        std::filesystem::path path(filePath);
        std::string breed_dir = path.parent_path().string();  // e.g. "models/corgi"
        std::string texturePath = breed_dir + "/textures/albedo.png";
        mat.albedo = std::make_unique<VeTexture>(device, assetManager, texturePath);
        mat.albedoInfo = VkDescriptorImageInfo{mat.albedo->getSampler(), mat.albedo->getImageView(), mat.albedo->getLayout()};
        mat.materialSetLayout = VeDescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                .build();
            VeDescriptorWriter(*mat.materialSetLayout, descriptorPool)
                    .writeImage(0,&mat.albedoInfo, 1)
                    .build(mat.materialDescriptorSets);
        model->materialComponent = std::make_unique<MaterialComponent>(std::move(mat));
        return model;
    }
    std::unique_ptr<VeModel> VeModel::createCubeMap(VeDevice& device, glm::vec3 cubeVetices[CUBE_MAP_VERTEX_COUNT]){
        Builder builder{};
        builder.loadCubeMap(cubeVetices);
        return std::make_unique<VeModel>(device, builder);
    }
    std::unique_ptr<VeModel> VeModel::createQuad(VeDevice& device){
        Builder builder{};
        builder.loadQuad();
        return std::make_unique<VeModel>(device, builder);
    }
//    void VeModel::Builder::loadModel(const std::string& filePath, AAssetManager *assetManager){
//        tinyobj::attrib_t attrib;
//        std::vector<tinyobj::shape_t> shapes;
//        std::vector<tinyobj::material_t> materials;
//        std::string warn;
//        std::string err;
//        std::vector<uint8_t> fileData = ve::loadBinaryFileToVector(filePath.c_str(), assetManager);
//        if(fileData.empty()){
//            std::cerr<<"Error loading obj file data"<<std::endl;
//            throw std::runtime_error("OBJ file loading failed");
//        }
//        std::string fileString(fileData.begin(), fileData.end());
//        std::stringbuf objBuf(fileString);
//        std::istream inputStream(&objBuf);
//        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream);
//        if(!ret){
//            std::cerr << "Failed to load OBJ: " << warn << " " << err << std::endl;
//            throw std::runtime_error("OBJ file parsing failed");
//        }
//        vertices.clear();
//        indices.clear();
//        std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};
//        for(const auto& shape: shapes){
//            for(const auto& index: shape.mesh.indices){
//                Vertex vertex{};
//                if(index.vertex_index >= 0){
//                    vertex.position = {
//                        attrib.vertices[3 * index.vertex_index + 0],
//                        attrib.vertices[3 * index.vertex_index + 1],
//                        attrib.vertices[3 * index.vertex_index + 2]
//                    };
//                }
//                vertex.color = {
//                    attrib.colors[3 * index.vertex_index + 0],
//                    attrib.colors[3 * index.vertex_index + 1],
//                    attrib.colors[3 * index.vertex_index + 2]
//                };
//                if(index.normal_index >= 0){
//                    vertex.normal = {
//                        attrib.normals[3 * index.normal_index + 0],
//                        attrib.normals[3 * index.normal_index + 1],
//                        attrib.normals[3 * index.normal_index + 2]
//                    };
//                }
//                if(index.texcoord_index >= 0){
//                    vertex.uv = {
//                        attrib.texcoords[2 * index.texcoord_index + 0],
//                        attrib.texcoords[2 * index.texcoord_index + 1],
//                    };
//                }
//                vertex.jointIndices = glm::ivec4(0);
//                vertex.jointWeights = glm::vec4(1.0f,0.0f,0.0f,0.0f);
//
//                //if vertex is unique, add to list

//                if(uniqueVertices.count(vertex) == 0){
//                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
//                    vertices.push_back(vertex);
//                }
//                indices.push_back(uniqueVertices[vertex]);
//            }
//            //compute tangents for each triangle (access 3 vertices at each loop step)
//            for (long index = 0;index < indices.size(); index+=3){
//                //edges: edge1 = vertex2 - vertex1, edge2 = vertex3 - vertex1
//                glm::vec3 edge1 = vertices[indices[index + 1]].position - vertices[indices[index]].position;
//                glm::vec3 edge2 = vertices[indices[index + 2]].position - vertices[indices[index]].position;
//                //delta uv: deltaUV1 = uv2 - uv1, deltaUV2 = uv3 - uv1
//                glm::vec2 deltaUV1 = vertices[indices[index + 1]].uv - vertices[indices[index]].uv;
//                glm::vec2 deltaUV2 = vertices[indices[index + 2]].uv - vertices[indices[index]].uv;
//                //compute tangent
//                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
//                glm::vec3 tangent;
//                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
//                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
//                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
//                //store tangent for each triangle
//                vertices[indices[index]].tangent = tangent;
//                vertices[indices[index + 1]].tangent = tangent;
//                vertices[indices[index + 2]].tangent = tangent;
//
//            }
//        }
//    }
//
    void VeModel::Builder::loadModelGLTF(const std::string& filePath, AAssetManager *assetManager){
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        bool ret = false;

        tinygltf::asset_manager = assetManager;
        // Check if file is binary (.glb) or text (.gltf) format
        if (filePath.find(".glb") != std::string::npos) {
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
        } else {
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
        }
        
        if(!warn.empty()){
            LOGE("Warning: %s", warn.c_str());
        }
        if(!err.empty()){
            LOGE("Error: %s", err.c_str());
        }
        if(!ret){
            LOGE("Failed to load gltf file");
        }
        // //load buffers
        // for(const auto& buffer: model.buffers){
        //     std::cout << "Buffer: " << buffer.name << std::endl;
        // }
        // //load images
        // for(const auto& image: model.images){
        //     std::cout << "Image: " << image.name << std::endl;
        // }
        // //load materials
        // for(const auto& material: model.materials){
        //     std::cout << "Material: " << material.name << std::endl;
        // }
        // //load meshes
        // for(const auto& mesh: model.meshes){
        //     std::cout << "Mesh: " << mesh.name << std::endl;
        // }
        // //load nodes
        // for(const auto& node: model.nodes){
        //     std::cout << "Node: " << node.name << std::endl;
        // }
        // //load textures
        // for(const auto& texture: model.textures){
        //     std::cout << "Texture: " << texture.name << std::endl;
        // }

        vertices.clear();
        indices.clear();
        std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};
        for (const auto& mesh : model.meshes) {
            for (const auto& primitive : mesh.primitives) {
                // Get accessor indices for the attributes we need
                const auto& positionAccessorIt = primitive.attributes.find("POSITION");
                const auto& normalAccessorIt = primitive.attributes.find("NORMAL");
                const auto& texcoordAccessorIt = primitive.attributes.find("TEXCOORD_0");
                const auto& colorAccessorIt = primitive.attributes.find("COLOR_0");
                const auto& jointsAccessorIt = primitive.attributes.find("JOINTS_0");
                const auto& weightsAccessorIt = primitive.attributes.find("WEIGHTS_0");
                
                // Check if we have position data (required)
                if (positionAccessorIt == primitive.attributes.end()) {
                    continue;
                }
                
                // Get indices
                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                
                // Get position data
                const tinygltf::Accessor& posAccessor = model.accessors[positionAccessorIt->second];
                const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
                
                // Optional: Get normal data if available
                const tinygltf::Accessor* normAccessor = nullptr;
                const tinygltf::BufferView* normBufferView = nullptr;
                const tinygltf::Buffer* normBuffer = nullptr;
                if (normalAccessorIt != primitive.attributes.end()) {
                    normAccessor = &model.accessors[normalAccessorIt->second];
                    normBufferView = &model.bufferViews[normAccessor->bufferView];
                    normBuffer = &model.buffers[normBufferView->buffer];
                }
                
                // Optional: Get texcoord data if available
                const tinygltf::Accessor* texAccessor = nullptr;
                const tinygltf::BufferView* texBufferView = nullptr;
                const tinygltf::Buffer* texBuffer = nullptr;
                if (texcoordAccessorIt != primitive.attributes.end()) {
                    texAccessor = &model.accessors[texcoordAccessorIt->second];
                    texBufferView = &model.bufferViews[texAccessor->bufferView];
                    texBuffer = &model.buffers[texBufferView->buffer];
                }
                
                // Optional: Get color data if available
                const tinygltf::Accessor* colorAccessor = nullptr;
                const tinygltf::BufferView* colorBufferView = nullptr;
                const tinygltf::Buffer* colorBuffer = nullptr;
                if (colorAccessorIt != primitive.attributes.end()) {
                    colorAccessor = &model.accessors[colorAccessorIt->second];
                    colorBufferView = &model.bufferViews[colorAccessor->bufferView];
                    colorBuffer = &model.buffers[colorBufferView->buffer];
                }
                
                 // New - get joint index data if available
                const tinygltf::Accessor* jointsAccessor = nullptr;
                const tinygltf::BufferView* jointsBufferView = nullptr;
                const tinygltf::Buffer* jointsBuffer = nullptr;
                if (jointsAccessorIt != primitive.attributes.end()) {
                    jointsAccessor = &model.accessors[jointsAccessorIt->second];
                    jointsBufferView = &model.bufferViews[jointsAccessor->bufferView];
                    jointsBuffer = &model.buffers[jointsBufferView->buffer];
                }
                
                // New - get joint weight data if available
                const tinygltf::Accessor* weightsAccessor = nullptr;
                const tinygltf::BufferView* weightsBufferView = nullptr;
                const tinygltf::Buffer* weightsBuffer = nullptr;
                if (weightsAccessorIt != primitive.attributes.end()) {
                    weightsAccessor = &model.accessors[weightsAccessorIt->second];
                    weightsBufferView = &model.bufferViews[weightsAccessor->bufferView];
                    weightsBuffer = &model.buffers[weightsBufferView->buffer];
                }
                
                // Number of vertices
                size_t vertexTotalCount = posAccessor.count;
                
                // Create temporary vertices
                std::vector<Vertex> tempVertices(vertexTotalCount);
                
                // Load position data
                const float* posData = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
                for (size_t i = 0; i < vertexTotalCount; i++) {
                    tempVertices[i].position = {
                        posData[i * 3 + 0],
                        posData[i * 3 + 1],
                        posData[i * 3 + 2]
                    };
                }
                
                // Load normal data if available
                if (normAccessor) {
                    const float* normData = reinterpret_cast<const float*>(&normBuffer->data[normBufferView->byteOffset + normAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].normal = {
                            normData[i * 3 + 0],
                            normData[i * 3 + 1],
                            normData[i * 3 + 2]
                        };
                    }
                } else {
                    // Default normals
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].normal = { 0.0f, 1.0f, 0.0f };
                    }
                }
                
                // Load texcoord data if available
                if (texAccessor) {
                    const float* texData = reinterpret_cast<const float*>(&texBuffer->data[texBufferView->byteOffset + texAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].uv = {
                            texData[i * 2 + 0],
                            texData[i * 2 + 1]
                        };
                    }
                } else {
                    // Default UVs
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].uv = { 0.0f, 0.0f };
                    }
                }
                
                // Load color data if available
                if (colorAccessor) {
                    const float* colorData = reinterpret_cast<const float*>(&colorBuffer->data[colorBufferView->byteOffset + colorAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        if (colorAccessor->type == TINYGLTF_TYPE_VEC3) {
                            tempVertices[i].color = {
                                colorData[i * 3 + 0],
                                colorData[i * 3 + 1],
                                colorData[i * 3 + 2]
                            };
                        } else if (colorAccessor->type == TINYGLTF_TYPE_VEC4) {
                            // If color is vec4, just use the RGB components
                            tempVertices[i].color = {
                                colorData[i * 4 + 0],
                                colorData[i * 4 + 1],
                                colorData[i * 4 + 2]
                            };
                        }
                    }
                } else {
                    // Default colors
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].color = { 1.0f, 1.0f, 1.0f };
                    }
                }
                // New - Load joint indices if available
                if (jointsAccessor) {
                    // Joint indices are usually stored as vec4 of unsigned bytes or unsigned shorts
                    if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                        const uint8_t* jointsData = reinterpret_cast<const uint8_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
                        const int8_t* jointsData = reinterpret_cast<const int8_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* jointsData = reinterpret_cast<const uint16_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
                        const int16_t* jointsData = reinterpret_cast<const int16_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        const uint32_t* jointsData = reinterpret_cast<const uint32_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                static_cast<int>(jointsData[i * 4 + 0]),
                                static_cast<int>(jointsData[i * 4 + 1]),
                                static_cast<int>(jointsData[i * 4 + 2]),
                                static_cast<int>(jointsData[i * 4 + 3])
                            };
                        }
                    } else if (jointsAccessor->componentType == TINYGLTF_COMPONENT_TYPE_INT) {
                        const int32_t* jointsData = reinterpret_cast<const int32_t*>(
                            &jointsBuffer->data[jointsBufferView->byteOffset + jointsAccessor->byteOffset]);
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {
                                jointsData[i * 4 + 0],
                                jointsData[i * 4 + 1],
                                jointsData[i * 4 + 2],
                                jointsData[i * 4 + 3]
                            };
                        }
                    } else {
                        for (size_t i = 0; i < vertexTotalCount; i++) {
                            tempVertices[i].jointIndices = {0, 0, 0, 0}; // Or whatever default makes sense
                        }
                    }

                }else {
                    // Default joint indices if attribute is not present
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].jointIndices = {0, 0, 0, 0}; // Or whatever default makes sense
                    }
                }
            
                // New - Load joint weights if available
                if (weightsAccessor) {
                    const float* weightsData = reinterpret_cast<const float*>(
                        &weightsBuffer->data[weightsBufferView->byteOffset + weightsAccessor->byteOffset]);
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        tempVertices[i].jointWeights = {
                            weightsData[i * 4 + 0],
                            weightsData[i * 4 + 1],
                            weightsData[i * 4 + 2],
                            weightsData[i * 4 + 3]
                        };
                        
                        // Normalize weights to ensure they sum to 1.0
                        float sum = tempVertices[i].jointWeights.x + 
                                    tempVertices[i].jointWeights.y + 
                                    tempVertices[i].jointWeights.z + 
                                    tempVertices[i].jointWeights.w;
                        
                        if (sum > 0.0f) {
                            tempVertices[i].jointWeights /= sum;
                        } else {
                            // If no weights, assign fully to the first joint
                            tempVertices[i].jointWeights = { 1.0f, 0.0f, 0.0f, 0.0f };
                        }
                    }
                }else {
                    // Default joint weights if attribute is not present
                    for (size_t i = 0; i < vertexTotalCount; i++) {
                        // Weight for the first joint is 1.0, others 0.0
                        tempVertices[i].jointWeights = {1.0f, 0.0f, 0.0f, 0.0f};
                    }
                }
                // Load indices
                std::vector<uint32_t> tempIndices;
                const uint8_t* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
                
                // Handle different index component types
                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* shortIndices = reinterpret_cast<const uint16_t*>(indexData);
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(static_cast<uint32_t>(shortIndices[i]));
                    }
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* intIndices = reinterpret_cast<const uint32_t*>(indexData);
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(intIndices[i]);
                    }
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* byteIndices = indexData;
                    for (size_t i = 0; i < indexAccessor.count; i++) {
                        tempIndices.push_back(static_cast<uint32_t>(byteIndices[i]));
                    }
                }
                
                // Add vertices and indices to the model
                for (size_t i = 0; i < tempIndices.size(); i++) {
                    Vertex vertex = tempVertices[tempIndices[i]];
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices[vertex]);
                }
            }
        }

        // Compute tangents for normal mapping
        for (size_t i = 0; i < indices.size(); i += 3) {
            if (i + 2 >= indices.size()) break; // Good boundary check

            Vertex& v0 = vertices[indices[i]];
            Vertex& v1 = vertices[indices[i+1]];
            Vertex& v2 = vertices[indices[i+2]];

            glm::vec3 edge1 = v1.position - v0.position;
            glm::vec3 edge2 = v2.position - v0.position;

            glm::vec2 deltaUV1 = v1.uv - v0.uv;
            glm::vec2 deltaUV2 = v2.uv - v0.uv;

            float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

            glm::vec3 tangent;

            if (std::abs(det) < 1e-6f) { // Check if determinant is close to zero
                // UVs are degenerate or co-linear, cannot compute tangent in the standard way.
                // Create an orthonormal basis with the normal.
                // Pick an arbitrary vector not parallel to the normal.
                glm::vec3 helper = (std::abs(v0.normal.x) > 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
                tangent = glm::normalize(glm::cross(v0.normal, helper));
                // (Optionally compute bitangent: glm::vec3 bitangent = glm::normalize(glm::cross(v0.normal, tangent));)
            } else {
                float f = 1.0f / det;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                tangent = glm::normalize(tangent); // Normalize the tangent
            }

            v0.tangent = tangent;
            v1.tangent = tangent; // Assigning the same tangent to all three vertices of the triangle
            v2.tangent = tangent; // This is a common simplification. For better quality, average tangents at shared vertices.
        }
    }

    void VeModel::Builder::loadCubeMap(glm::vec3 cubeVertices[CUBE_MAP_VERTEX_COUNT]){
        vertices.clear();
        indices.clear();
        std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};
        for (size_t i = 0; i < CUBE_MAP_VERTEX_COUNT; i++) {
            Vertex vertex{};
            vertex.position = cubeVertices[i];
            vertex.color = { 1.0f, 1.0f, 1.0f };
            vertex.normal = { 0.0f, 0.0f, 0.0f };
            vertex.uv = { 0.0f, 0.0f };

            vertex.jointIndices = glm::ivec4(0);
            vertex.jointWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
    void VeModel::Builder::loadQuad(){
        vertices.clear();
        indices.clear();
        std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};

        // Define quad vertices
        glm::vec3 quadVertices[4] = {
                { -1.0f, 0.0f,  1.0f }, // Front-left
                {  1.0f, 0.0f,  1.0f }, // Front-right
                { -1.0f, 0.0f, -1.0f }, // Back-left
                {  1.0f, 0.0f, -1.0f }  // Back-right
        };

        // Tile the UV coordinates - this creates 8x8 tiles
        float tileCount = 4.0f;
        float uvs[4][2] = {
                {0.0f, 0.0f},              // Front-left
                {tileCount, 0.0f},         // Front-right
                {0.0f, tileCount},         // Back-left
                {tileCount, tileCount}     // Back-right
        };

        for (size_t i = 0; i < 4; i++) {
            Vertex vertex{};
            vertex.position = quadVertices[i];
            vertex.color = { 1.0f, 1.0f, 1.0f };
            vertex.normal = { 0.0f, 1.0f, 0.0f };
            vertex.uv = { uvs[i][0], uvs[i][1] };  // Use the tiled UVs
            vertex.jointIndices = glm::ivec4(0);
            vertex.jointWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
        }
        indices = { 2, 3, 1, 2, 1, 0 };
    }
    void VeModel::loadSkeleton(const tinygltf::Model& model){
        size_t numSkeletons = model.skins.size();
        if(!numSkeletons)
            return;

        skeleton = std::make_unique<Skeleton>();
        
        tinygltf::Skin skin = model.skins[0];
        if(skin.inverseBindMatrices!=-1){
            auto& joints = skeleton -> joints; 
            size_t numJoints = skin.joints.size();
            joints.resize(numJoints);
            skeleton->name=skin.name;
            skeleton->jointMatrices.resize(numJoints);

            const tinygltf::Accessor& invAccessor = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView& invBufferView = model.bufferViews[invAccessor.bufferView];
            const tinygltf::Buffer& invBuffer = model.buffers[invBufferView.buffer];

            // Validate component type
            if (invAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                throw std::runtime_error("Inverse bind matrices are not of type float.");
            }

            // Get direct pointer to float data - no need for transposition
            const glm::mat4* inverseBindMatricesData = reinterpret_cast<const glm::mat4*>(
                &invBuffer.data[invBufferView.byteOffset + invAccessor.byteOffset]);
            
            // Process each joint
            for (size_t i = 0; i < numJoints; i++) {
                int jointNodeIdx = skin.joints[i];
                
                // Setup joint data
                joints[i].name = model.nodes[jointNodeIdx].name;
                skeleton->nodeJointMap[jointNodeIdx] = i;
                //  inverse bind matrix
                joints[i].inverseBindMatrix = inverseBindMatricesData[i];
                 // local world magtrix
                // extractNodeTransform(model.nodes[jointNodeIdx], joints[i]);
                // joints[i].jointWorldMatrix = calculateLocalTransform(joints[i]);


                //obtain TRS and localWorldMatrix
                auto& node = model.nodes[jointNodeIdx];
                if(node.translation.size()==3){
                    joints[i].translation = glm::make_vec3(node.translation.data());
                }
                if(node.rotation.size()==4){
                    glm::quat q = glm::make_quat(node.rotation.data());
                    joints[i].rotation = glm::mat4(q);
                }
                if(node.scale.size()==3){
                    joints[i].scale = glm::make_vec3(node.scale.data());
                }
                if(node.matrix.size()==16){
                    joints[i].jointWorldMatrix = glm::make_mat4x4(node.matrix.data());
                }else{
                    joints[i].jointWorldMatrix = glm::mat4(1.0f);
                }
                skeleton->nodeJointMap[jointNodeIdx] = i;
            }
            int rootJoint = skin.joints[0];
            loadJoints(rootJoint, -1, model);
            // updateJointHierarchy(model);
        }
        
    }
    void VeModel::loadJoints(int nodeIndex, int parentIndex, const tinygltf::Model& model){
        int currentJoint = skeleton->nodeJointMap[nodeIndex];
        auto& joint = skeleton->joints[currentJoint];
        joint.parentIndex = parentIndex;
        size_t numChildren = model.nodes[nodeIndex].children.size();
        if(numChildren>0){
            joint.childrenIndices.resize(numChildren);
            for(size_t i = 0; i < numChildren; i++){
                int childNodeIndex = model.nodes[nodeIndex].children[i];
                joint.childrenIndices[i] = skeleton->nodeJointMap[childNodeIndex];
                loadJoints(childNodeIndex, currentJoint, model);
            }
        }
    }
    void VeModel::loadAnimations(const tinygltf::Model& model){
        if(!skeleton){
            LOGE("Error: Skeleton not loaded");
            return;
        }
        size_t numAnimations = model.animations.size();
        if(!numAnimations){
            LOGE("Error: No animations found");
            return;
        }
        animationManager = std::make_shared<AnimationManager>();
        for(size_t i = 0; i < numAnimations; i++){
            const tinygltf::Animation& animation = model.animations[i];
            std::string name = animation.name.empty() ? "animation" + std::to_string(i) : animation.name;
            std::shared_ptr<Animation> anim = std::make_shared<Animation>(name);
            //samplers
            size_t numSamplers = animation.samplers.size();
            anim->samplers.resize(numSamplers);
            for(size_t samplerIndex = 0; samplerIndex < numSamplers; samplerIndex++) {
                tinygltf::AnimationSampler gltfSampler = animation.samplers[samplerIndex];
                auto& sampler = anim->samplers[samplerIndex];
                sampler.interpolationMethod = Animation::InterpolationMethod::LINEAR;
                if(gltfSampler.interpolation == "STEP")
                    sampler.interpolationMethod = Animation::InterpolationMethod::STEP;
                
                // Process input timestamps
                const tinygltf::Accessor& inputAccessor = model.accessors[gltfSampler.input];
                const tinygltf::BufferView& inputBufferView = model.bufferViews[inputAccessor.bufferView];
                const tinygltf::Buffer& inputBuffer = model.buffers[inputBufferView.buffer];
                
                size_t count = inputAccessor.count;
                sampler.timeStamps.resize(count);
                
                // Get byte stride
                size_t inputByteStride = inputBufferView.byteStride;
                if (inputByteStride == 0) {
                    // If byteStride is not defined, calculate it based on the accessor's type and component type
                    inputByteStride = tinygltf::GetComponentSizeInBytes(inputAccessor.componentType) * 
                                    tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_SCALAR);
                }
                
                // Get buffer data pointer
                const unsigned char* inputBufferData = &inputBuffer.data[inputBufferView.byteOffset + 
                                                    inputAccessor.byteOffset];
                
                if (inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    for (size_t i = 0; i < count; i++) {
                        const float* value = reinterpret_cast<const float*>(inputBufferData + i * inputByteStride);
                        sampler.timeStamps[i] = *value;
                    }
                } else {
                    LOGE("Error: Animation sampler input component type not supported");
                }
                
                // Process output values
                const tinygltf::Accessor& outputAccessor = model.accessors[gltfSampler.output];
                const tinygltf::BufferView& outputBufferView = model.bufferViews[outputAccessor.bufferView];
                const tinygltf::Buffer& outputBuffer = model.buffers[outputBufferView.buffer];
                
                count = outputAccessor.count;
                sampler.TRSoutputValues.resize(count);
                    
                // Get byte stride
                size_t outputByteStride = outputBufferView.byteStride;
                if (outputByteStride == 0) {
                    // If byteStride is not defined, calculate it based on the accessor's type and component type
                    outputByteStride = tinygltf::GetComponentSizeInBytes(outputAccessor.componentType) * 
                                    tinygltf::GetNumComponentsInType(outputAccessor.type);
                }
                
                // Get buffer data pointer
                const unsigned char* outputBufferData = &outputBuffer.data[outputBufferView.byteOffset + 
                                                        outputAccessor.byteOffset];
                
                switch (outputAccessor.type) {
                    case TINYGLTF_TYPE_VEC3:
                    {
                        for (size_t i = 0; i < count; i++) {
                            const float* value = reinterpret_cast<const float*>(outputBufferData + i * outputByteStride);
                            sampler.TRSoutputValues[i] = glm::vec4(value[0], value[1], value[2], 0.0f);
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4:
                    {
                        for (size_t i = 0; i < count; i++) {
                            const float* value = reinterpret_cast<const float*>(outputBufferData + i * outputByteStride);
                            sampler.TRSoutputValues[i] = glm::vec4(value[0], value[1], value[2], value[3]);
                        }
                        break;
                    }
                    default:
                    {
                        LOGE("Error: Animation sampler output type not supported");
                        break;
                    }
                }
            }
            
      
            //atleast one samplers exist
            if (anim->samplers.size()){
                auto& sampler = anim->samplers[0];
                //interpolate between two or more time stamps
                if(sampler.timeStamps.size() >=2){
                    anim->setFirstKeyFrameTime(sampler.timeStamps[0]);
                    anim->setLastKeyFrameTime(sampler.timeStamps.back());
                }
            }
            //each node of the skeleton has channels that point to a sampler
            size_t numChannels = animation.channels.size();
            anim->channels.resize(numChannels);
            for(size_t channelIndex = 0; channelIndex < numChannels; channelIndex++){
                tinygltf::AnimationChannel gltfChannel = animation.channels[channelIndex];
                auto& channel = anim->channels[channelIndex];
                channel.node = gltfChannel.target_node;
                channel.samplerIndex = gltfChannel.sampler;
                if(gltfChannel.target_path == "translation"){
                    channel.pathType = Animation::PathType::TRANSLATION;
                }else if(gltfChannel.target_path == "rotation"){
                    channel.pathType = Animation::PathType::ROTATION;
                }else if(gltfChannel.target_path == "scale"){
                    channel.pathType = Animation::PathType::SCALE;
                }else{
                    LOGE("Unknown channel target path: %s", gltfChannel.target_path.c_str());
                }
            }
            animationManager->push(anim);
            LOGI("Animation loaded: %s", anim->getName().c_str());
        }
        this->hasAnimation = (animationManager->size()) ? true : false;
    }
    // Extract translation, rotation, and scale from a node
    void VeModel::extractNodeTransform(const tinygltf::Node& node, Joint& joint) {
        // Default values
        joint.translation = glm::vec3(0.0f);
        joint.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        joint.scale = glm::vec3(1.0f);

        // If matrix is directly provided
        if (!node.matrix.empty()) {
            glm::mat4 nodeMatrix = glm::make_mat4(node.matrix.data());
            
            // Decompose the matrix into TRS components
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(nodeMatrix, joint.scale, joint.rotation, joint.translation, skew, perspective);
        } else {
            // Extract individual TRS components if provided
            if (!node.translation.empty()) {
                joint.translation = glm::vec3(
                    node.translation[0], 
                    node.translation[1], 
                    node.translation[2]
                );
            }
        
            if (!node.rotation.empty()) {
                joint.rotation = glm::quat(
                    node.rotation[3], // w is last in glTF but first in glm
                    node.rotation[0], 
                    node.rotation[1], 
                    node.rotation[2]
                );
            }
            
            if (!node.scale.empty()) {
                joint.scale = glm::vec3(
                    node.scale[0], 
                    node.scale[1], 
                    node.scale[2]
                );
            }
        }
    }

    // Calculate local transform matrix from TRS components
    glm::mat4 VeModel::calculateLocalTransform(const Joint& joint) {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), joint.translation);
        glm::mat4 rotationMatrix = glm::mat4_cast(joint.rotation);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), joint.scale);
        
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    // Update the joint hierarchy to calculate world matrices
    void VeModel::updateJointHierarchy(const tinygltf::Model& model) {
        // Find root joints (joints with parentIndex == -1)
        for (size_t i = 0; i < skeleton->joints.size(); i++) {
            if (skeleton->joints[i].parentIndex == -1) {
                // For root joints, local transform is the world transform
                skeleton->joints[i].jointWorldMatrix = calculateLocalTransform(skeleton->joints[i]);
                
                // Process children
                updateJointWorldMatrices(i);
            }
        }
    }

    // Recursively update joint world matrices
    void VeModel::updateJointWorldMatrices(int jointIndex) {
        Joint& joint = skeleton->joints[jointIndex];
        
        // Process all children
        for (int childIndex : joint.childrenIndices) {
            Joint& childJoint = skeleton->joints[childIndex];
            
            // Child's world matrix = parent's world matrix * child's local matrix
            glm::mat4 childLocalMatrix = calculateLocalTransform(childJoint);
            childJoint.jointWorldMatrix = joint.jointWorldMatrix * childLocalMatrix;
            
            // Recursively process this child's children
            updateJointWorldMatrices(childIndex);
        }
    }


}