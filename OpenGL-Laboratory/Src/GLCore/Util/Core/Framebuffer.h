#pragma once

#include "GLCore/Core/Core.h"

namespace GLCore
{
	namespace Utils
	{
		enum class FramebufferTextureFormat
		{
			None = 0,
			// Depth/stencil

			// Color
			RGBA8,
			RGBA32F,
			RED_INTEGER,
			RED_FLOAT,

			DEPTH24STENCIL8,

			// Defaults
			Depth = DEPTH24STENCIL8
		};

		struct FramebufferTextureSpecification
		{
			FramebufferTextureSpecification () = default;
			FramebufferTextureSpecification (FramebufferTextureFormat format)
				: TextureFormat (format)
			{}

			FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
			// TODO: filtering/wrap
		};

		struct FramebufferAttachmentSpecification
		{
			FramebufferAttachmentSpecification () = default;
			FramebufferAttachmentSpecification (std::initializer_list<FramebufferTextureSpecification> attachments)
				: Attachments (attachments)
			{}

			std::vector<FramebufferTextureSpecification> Attachments;
		};

		struct FramebufferSpecification
		{
			uint32_t Width = 0, Height = 0;
			FramebufferAttachmentSpecification Attachments;
			uint32_t Samples = 1;

			bool SwapChainTarget = false;
		};

		class Framebuffer
		{
		public:
			Framebuffer (const FramebufferSpecification &spec);
			virtual ~Framebuffer ();

			virtual void Bind ();
			virtual void Unbind ();

			virtual void Resize (uint32_t width, uint32_t height);

			void ReadPixel (uint32_t attachmentIndex, int x, int y, void* cantainer);

			virtual void ClearAttachment (uint32_t attachmentIndex, int value);

			virtual uint32_t GetColorAttachmentRendererID (uint32_t index = 0) const;

			virtual const FramebufferSpecification &GetSpecification () const { return m_Specification; };

			static std::shared_ptr<Framebuffer> Create (const FramebufferSpecification &spec);
		private:
			void Invalidate ();
		private:
			uint32_t m_RendererID = 0;
			FramebufferSpecification m_Specification;

			std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications; // That Last one is for Depth
			FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

			std::vector<uint32_t> m_ColorAttachments;
			uint32_t m_DepthAttachment = 0;
		};
	}
}