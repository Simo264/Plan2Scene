#pragma once

#include "../types.hpp"

#include <filesystem>
#include <string>

using ShaderObjectID = u32;
using ShaderProgramID = u32;
using ProgramPipelineID = u32;

enum class ShaderStage : i32
{ 
  Vertex          = 0x8B31,  // GL_VERTEX_SHADER
  TessControl     = 0x8E88,  // GL_TESS_CONTROL_SHADER
  TessEvaluation  = 0x8E87,  // GL_TESS_EVALUATION_SHADER
  Geometry        = 0x8DD9,  // GL_GEOMETRY_SHADER
  Fragment        = 0x8B30,  // GL_FRAGMENT_SHADER
};

enum class PipelineStage : u32
{
	VertexShader 		= 0x00000001, // GL_VERTEX_SHADER_BIT
	TessControl 		= 0x00000008, // GL_TESS_CONTROL_SHADER_BIT
	TessEvaluation 	= 0x00000010, // GL_TESS_EVALUATION_SHADER_BIT
	GeometryShader 	= 0x00000004, // GL_GEOMETRY_SHADER_BIT
	FragmentShader 	= 0x00000002, // GL_FRAGMENT_SHADER_BIT
	ComputeShader 	= 0x00000020, // GL_COMPUTE_SHADER_BIT
	All 						= 0xFFFFFFFF, // GL_ALL_SHADER_BITS
};


// Example of usage:
// 
// auto shader = Shader{};
// shader.create(ShaderType::Vertex);
// shader.load_source_code(SHADERS_DIR / "basic_shader.vert.glsl");
// shader.compile();
// if(!shader.check_compile_status())
// {
//   auto log = shader.get_compile_log();
//   std::println("Shader compilation error: {}", log);
// }
class ShaderObject 
{
public:
  ShaderObject() : m_id{ 0 } {} // The default value is 0, which is not a valid shader object ID
  
  // Creates an empty shader object and returns a non-zero value
  void create(ShaderStage stage);
  // Frees the memory and invalidates the name associated with the shader object 
  void destroy();
  // Load shader source code from a GLSL file
  void load_source_code(const std::filesystem::path& glsl_file) const;
  // Compile the shader
  void compile() const;
  // Check the compile status of the shader
  bool check_compile_status() const;
  // Get the compile log of the shader
  std::string get_compile_log() const;
  // Utility method to check if the shader is a valid object
  bool is_valid() const;
  
  auto id() const { return m_id; };
  
private:
  ShaderObjectID m_id;
};


// Example usage:
// 
// auto program = ShaderProgram{};
// program.create();
// program.attach_shader(vertex_shader_obj);
// program.attach_shader(fragment_shader_obj);
// program.link();
// status = program.check_link_status();
// if(!status){ ... }
// 
// // Always detach shaders after a successful link.
// program.detach_shader(vertex_shader_obj);
// program.detach_shader(fragment_shader_obj);
class ShaderProgram
{
public:
  ShaderProgram() : m_id{ 0 } {} // The default value is 0, which is not a valid program ID
  
  // Creates an empty program object and returns a non-zero value
  void create();
  // Frees the memory and invalidates the name associated with the program object 
  void destroy();
  // After creating a program, the shader objects you wish to link to it must be attached to the program
  void attach_shader(ShaderObject shader) const;
  // After linking (whether successfully or not), it is a good idea to detach all shader objects from the program
  void detach_shader(ShaderObject shader) const;
  // Links the program object 
  void link() const;
  // Linking can fail for many reasons. Program link failure can be detected and responded to, in a similar way to shader compilation failure.
  bool check_link_status() const;  
  // Returns the link log of the program
  std::string get_link_log() const;
  // Installs a program object as part of current rendering state
  void use() const;
  // To signal that a program object is intended to be used with this separate program model, we must set a parameter on the program before linking. 
  void set_separable(bool flag) const;
 
  // Returns the value of a uniform variable with added safety checks to prevent buffer overflow.
  // The `size` is parameter is the size of the buffer in bytes, not the number of elements.
  // The `value_out` parameter is a pointer to a buffer where the uniform's value(s) will be stored
  void get_uniform_value_f32(i32 location, i64 size, f32* value_out) const;
  void get_uniform_value_i32(i32 location, i64 size, i32* value_out) const;
  // Returns an integer that represents the location of a specific uniform variable within a program object
  i32 get_uniform_location(std::string_view name) const;
  // Specify the value of a uniform variable for the current program object
  void set_uniform_32(i32 location, f32 value) const;
  void set_uniform_vector3f(i32 location, const f32* value) const;
  void set_uniform_mat4f(i32 location, const f32* value) const;
 
 	// Utility method to check if the program is a valid object 
  bool is_valid() const;
  
  auto id() const { return m_id; };
  
private:
  ShaderProgramID m_id;
};

// Example of usage:
// 
// auto program_vert = ShaderProgram{};
// program_vert.create();
// program_vert.attach_shader(vertex_shader_obj);
// program_vert.set_separable(true); 
// program_vert.link();
// program_vert.detach_shader(vertex_shader_obj);
// 
// auto program_frag = ShaderProgram{};
// program_frag.create();
// program_frag.attach_shader(fragment_shader_obj);
// program_frag.set_separable(true); 
// program_frag.link();
// program_frag.detach_shader(fragment_shader_obj);
//
// auto pipeline = ProgramPipelineObject{};
// pipeline.create();
// pipeline.bind_program_stage(PipelineStage::VertexShader, program_vert);
// pipeline.bind_program_stage(PipelineStage::FragmentShader, program_frag);
// auto status = pipeline.validate_pipeline();
// if(!status){ ... }
class ProgramPipelineObject
{
public:
	ProgramPipelineObject() : m_id{ 0 } {} // The default value is 0, which is not a valid pipeline object ID
	
	// Create program pipeline object
	void create();
	// Delete program pipeline object
	void destroy();
	
	// Binds executables from a program object associated with a specified set of shader stages to the program pipeline object.
	// If program refers to a program object with a valid shader attached for an indicated shader stage,
	// installs the executable code for that stage in the indicated program pipeline object.
	void bind_program_stage(PipelineStage stage, ShaderProgram program) const;
	// Bind a program pipeline to the current context
	void bind() const;
	// Unbind a program pipeline to the current context
	void unbind() const;
	
	// Is used to validate a program pipeline object against the current OpenGL state. 
	// This allows the implementation to check for potential issues and perform internal optimizations to ensure 
	// the shaders in the pipeline will operate correctly.
	bool validate_pipeline() const;
	std::string get_validation_status() const;
	
	// Sets the linked program named by program to be the active program for the pipeline object.
	// The active program in the active pipeline object is the target of calls to glUniform.
	void set_active_program(ShaderProgram program) const;
	
	// Utility method to check if the pipeline is a valid object
	bool is_valid() const;
	
	auto id() const { return m_id; }
	
private:
	ProgramPipelineID m_id; 
};