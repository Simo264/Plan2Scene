#pragma once

#include "../types.hpp"

using BufferID = u32;

enum class BufferUsageFlags : u32
{
	DynamicStorage 	= 0x0100, // GL_DYNAMIC_STORAGE_BIT,
	MapRead 				= 0x0001, // GL_MAP_READ_BIT,
	MapWrite 				= 0x0002, // GL_MAP_WRITE_BIT,
	MapPersistent 	= 0x0040, // GL_MAP_PERSISTENT_BIT,
	MapCoherent 		= 0x0080, // GL_MAP_COHERENT_BIT,
	ClientStorage 	= 0x0200, // GL_CLIENT_STORAGE_BIT
};

class Buffer 
{
public:
	Buffer() : m_id{ 0 } {};
	
	// Create buffer object
	void create();
	// Delete buffer object
	void destroy();
	// Creates and initializes a buffer object's immutable data store
	void allocate_storage(i64 size, const void* data, BufferUsageFlags flags);
	// Updates a subset of a buffer object's data store.
	void update_data(i32 offset, u64 size, const void* data);
	
	// Utility for checking the validity of the buffer object
	bool is_valid() const;

	auto id() const { return m_id; }
private:
	BufferID m_id;
};