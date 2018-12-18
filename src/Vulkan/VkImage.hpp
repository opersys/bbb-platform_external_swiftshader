// Copyright 2018 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VK_IMAGE_HPP_
#define VK_IMAGE_HPP_

#include "VkObject.hpp"

namespace sw
{
	class Surface;
};

namespace vk
{

class Image : public Object<Image, VkImage>
{
public:
	Image(const VkImageCreateInfo* pCreateInfo, void* mem);
	~Image() = delete;
	void destroy(const VkAllocationCallbacks* pAllocator);

	static size_t ComputeRequiredAllocationSize(const VkImageCreateInfo* pCreateInfo);

	VkDeviceSize getStorageSize() const;
	const VkMemoryRequirements getMemoryRequirements() const;
	void bind(VkDeviceMemory pDeviceMemory, VkDeviceSize pMemoryOffset);
	void copyTo(VkImage dstImage, const VkImageCopy& pRegion);
	void copyTo(VkBuffer dstBuffer, const VkBufferImageCopy& pRegion);
	void copyFrom(VkBuffer srcBuffer, const VkBufferImageCopy& pRegion);

	void clear(const VkClearValue& clearValue, const VkRect2D& renderArea, const VkImageSubresourceRange& subresourceRange);

	VkImageType              getImageType() const { return imageType; }
	VkFormat                 getFormat() const { return format; }

private:
	void* getTexelPointer(const VkOffset3D& offset) const;
	VkDeviceSize texelOffsetBytesInStorage(const VkOffset3D& offset) const;
	int rowPitchBytes() const;
	int slicePitchBytes() const;
	int bytesPerTexel() const;
	int getBorder() const;

	VkDeviceMemory           deviceMemory = nullptr;
	VkDeviceSize             memoryOffset = 0;
	VkImageCreateFlags       flags = 0;
	VkImageType              imageType = VK_IMAGE_TYPE_2D;
	VkFormat                 format = VK_FORMAT_UNDEFINED;
	VkExtent3D               extent = {0, 0, 0};
	uint32_t                 mipLevels = 0;
	uint32_t                 arrayLayers = 0;
	VkSampleCountFlagBits    samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling            tiling = VK_IMAGE_TILING_OPTIMAL;
};

static inline Image* Cast(VkImage object)
{
	return reinterpret_cast<Image*>(object);
}

} // namespace vk

#endif // VK_IMAGE_HPP_