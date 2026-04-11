#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
namespace Runtime::Render::RHI
{

/**
 * @about AttachmentDescription
 */
inline static vk::AttachmentDescription CreateAttachmentDescription(vk::Format format, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
	return vk::AttachmentDescription(
		{},
		format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		initialLayout,
		finalLayout
	);
}

inline static vk::AttachmentDescription CreateTempAttachmentDescription(vk::Format format, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
	return vk::AttachmentDescription(
		{},
		format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		initialLayout,
		finalLayout
	);
}


} // namespace Runtime::Render::RHI