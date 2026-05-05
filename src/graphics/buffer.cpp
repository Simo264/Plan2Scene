#include "buffer.hpp"

#include <glad/gl.h>
#include <print>

void Buffer::create()
{
	glCreateBuffers(1, &m_id);
	if(!is_valid())
		std::println("Failed to create buffer object (id={})", m_id);
}

void Buffer::destroy()	
{
	if(!is_valid())
		return;
	
	glDeleteBuffers(1, &m_id);
	m_id = 0;
}
	
bool Buffer::is_valid() const
{
	return m_id != 0 && glIsBuffer(m_id) == GL_TRUE;
}

void Buffer::allocate_storage(i64 size, const void* data, BufferUsageFlags flags)
{
	if(!is_valid())
		return;
	
	glNamedBufferStorage(m_id, size, data, static_cast<u32>(flags));
}

void Buffer::update_data(i32 offset, u64 size, const void* data)
{
	if(!is_valid())
		return;
	
	glNamedBufferSubData(m_id, offset, size, data);
}