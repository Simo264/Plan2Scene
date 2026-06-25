#pragma once

#include "../types.hpp"
#include "texture.hpp"

enum class FramebufferTarget : u32
{
	DRAW = 0x8CA9, // GL_DRAW_FRAMEBUFFER
	READ = 0x8CA8, // GL_READ_FRAMEBUFFER
	READ_DRAW = 0x8D40  // GL_FRAMEBUFFER
};

enum class FramebufferAttachment : u32
{
	COLOR_0 = 0x8CE0, // GL_COLOR_ATTACHMENT0
	COLOR_1 = 0x8CE1, // GL_COLOR_ATTACHMENT1
	COLOR_2 = 0x8CE2, // GL_COLOR_ATTACHMENT2
	COLOR_3 = 0x8CE3, // GL_COLOR_ATTACHMENT3
	COLOR_4 = 0x8CE4, // GL_COLOR_ATTACHMENT4
	COLOR_5 = 0x8CE5, // GL_COLOR_ATTACHMENT5
	COLOR_6 = 0x8CE6, // GL_COLOR_ATTACHMENT6
	COLOR_7 = 0x8CE7, // GL_COLOR_ATTACHMENT7
	COLOR_8 = 0x8CE8, // GL_COLOR_ATTACHMENT8
	COLOR_9 = 0x8CE9, // GL_COLOR_ATTACHMENT9
	// ...
	// Color31		= GL_COLOR_ATTACHMENT31
	DEPTH = 0x8D00, // GL_DEPTH_ATTACHMENT
	STENCIL = 0x8D20, // GL_STENCIL_ATTACHMENT
	DEPTH_STENCIL = 0x821A  // GL_DEPTH_STENCIL_ATTACHMENT
};

enum class FramebufferBlitFilter : u32
{
	NEAREST = 0x2600, // GL_NEAREST
	LINEAR = 0x2601  // GL_LINEAR
};

enum class FramebufferBlitMask : u32
{
	COLOR_BUFFER = 0x00004000, // GL_COLOR_BUFFER_BIT
	DEPTH_BUFFER = 0x00000100, // GL_DEPTH_BUFFER_BIT
	STENCIL_BUFFER = 0x00000400, // GL_STENCIL_BUFFER_BIT
};

/**
 * @brief Framebuffer objects are a collection of attachments.
 * 
 * As standard OpenGL Objects, FBOs have the usual glGenFramebuffers and glDeleteFramebuffers functions. 
 * As expected, it also has the usual glBindFramebuffer function, to bind an FBO to the context.
 * 
 * The target​ parameter for this object can take one of 3 values: 
 * 1. GL_FRAMEBUFFER
 * 2. GL_READ_FRAMEBUFFER 
 * 3. GL_DRAW_FRAMEBUFFER
 * 
 * The last two allow you to bind an FBO so that reading commands (glReadPixels, etc) and writing commands 
 * (all rendering commands) can happen to two different framebuffers. 
 * The GL_FRAMEBUFFER binding target simply sets both the read and the write to the same FBO.
 *  
 * Each FBO image represents an attachment point, a location in the FBO where an image can be attached. 
 * FBOs have the following attachment points:
 * 	1. GL_COLOR_ATTACHMENTi:				these attachment points can only have images bound to them with color-renderable formats
 * 	2. GL_DEPTH_ATTACHMENT:					the image attached becomes the Depth Buffer for the FBO
 * 	3. GL_STENCIL_ATTACHMENT:				the image attached becomes the stencil buffer for the FBO
 * 	4. GL_DEPTH_STENCIL_ATTACHMENT:	the image attached becomes both the depth and stencil buffers
 *  
 * Renderbuffer objects contain images. They are created and used specifically with Framebuffer Objects. 
 * They are optimized for use as render targets, while Textures may not be, and are the logical choice 
 * when you do not need to sample from the produced image. 
 * If you need to resample, use Textures instead. 
 * Renderbuffer objects also natively accommodate Multisampling.
 */
class FrameBuffer
{
public:
	FrameBuffer() : id{ 0 } {}
	~FrameBuffer() = default;

	void create();
	void destroy();
	void bind(FramebufferTarget target) const;
	void unbind(FramebufferTarget target) const;
	bool check_status() const;

	void attach_texture(FramebufferAttachment attachment, Texture texture, i32 level);

	bool is_valid() const;

	u32 id;
};