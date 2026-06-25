#include "framebuffer.hpp"

#include <glad/gl.h>

void FrameBuffer::create()
{
	glCreateFramebuffers(1, &id);
}

void FrameBuffer::destroy()
{
	glDeleteFramebuffers(1, &id);
	id = 0;
}

void FrameBuffer::bind(FramebufferTarget target) const
{
	glBindFramebuffer(static_cast<u32>(target), id);
}

void FrameBuffer::unbind(FramebufferTarget target) const
{
	glBindFramebuffer(static_cast<u32>(target), 0);
}

bool FrameBuffer::check_status() const
{
	return glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool FrameBuffer::is_valid() const
{
	return (id != 0) && (glIsFramebuffer(id) == GL_TRUE);
}

void FrameBuffer::attach_texture(FramebufferAttachment attachment, Texture texture, i32 level)
{
	glNamedFramebufferTexture(id, static_cast<u32>(attachment), texture.id(), level);
}

