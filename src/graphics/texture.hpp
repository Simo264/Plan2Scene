#pragma once

#include "../types.hpp"

#include <filesystem>

enum class TextureType : u32
{
  Texture1D = 0x0DE0,                 // GL_TEXTURE_1D
  Texture2D = 0x0DE1,                 // GL_TEXTURE_2D
  Texture3D = 0x0DE2,                 // GL_TEXTURE_3D
  TextureRectangle = 0x0DE3,          // GL_TEXTURE_RECTANGLE
  TextureBuffer = 0x0DE4,             // GL_TEXTURE_BUFFER
  TextureCubeMap = 0x0DE5,            // GL_TEXTURE_CUBE_MAP
  Texture1DArray = 0x0DE6,            // GL_TEXTURE_1D_ARRAY
  Texture2DArray = 0x0DE7,            // GL_TEXTURE_2D_ARRAY
  TextureCubeMapArray = 0x0DE8,       // GL_TEXTURE_CUBE_MAP_ARRAY
  Texture2DMultisample = 0x0DE9,      // GL_TEXTURE_2D_MULTISAMPLE
  Texture2DMultisampleArray = 0x0DEA, // GL_TEXTURE_2D_MULTISAMPLE_ARRAY
};

enum class TextureImageFormat : u32 
{
  // --- Color normalized ---
  R8              = 0x8229,   // GL_R8
  RG8             = 0x822B,   // GL_RG8
  RGB8            = 0x8051,   // GL_RGB8
  RGBA8           = 0x8058,   // GL_RGBA8
  // --- Color float ---
  RGB16F          = 0x881B,   // GL_RGB16F
  RGBA16F         = 0x881A,   // GL_RGBA16F
  RGB32F          = 0x8815,   // GL_RGB32F
  RGBA32F         = 0x8814,   // GL_RGBA32F
  // --- sRGB ---
  SRGB8           = 0x8C41,   // GL_SRGB8
  SRGB8_Alpha8    = 0x8C43,   // GL_SRGB8_ALPHA8
  // --- Color integer ---
  RGB8I           = 0x8D8F,   // GL_RGB8I
  RGBA8I          = 0x8D8E,   // GL_RGBA8I
  RGB8UI          = 0x8D7D,   // GL_RGB8UI
  RGBA8UI         = 0x8D7C,   // GL_RGBA8UI
  // --- Depth ---
  Depth16         = 0x81A5,   // GL_DEPTH_COMPONENT16
  Depth24         = 0x81A6,   // GL_DEPTH_COMPONENT24
  Depth32         = 0x81A7,   // GL_DEPTH_COMPONENT32
  Depth32F        = 0x8CAC,   // GL_DEPTH_COMPONENT32F
  // --- Depth + Stencil ---
  Depth24Stencil8  = 0x88F0,  // GL_DEPTH24_STENCIL8
  Depth32FStencil8 = 0x8CAD,  // GL_DEPTH32F_STENCIL8
  // --- Stencil ---
  Stencil8        = 0x8D48,   // GL_STENCIL_INDEX8
};

enum class TextureWrapMode : u32
{
  Repeat = 0x2901,           // GL_REPEAT
  MirroredRepeat = 0x8370,   // GL_MIRRORED_REPEAT
  ClampToEdge = 0x812F,      // GL_CLAMP_TO_EDGE
  ClampToBorder = 0x812D,    // GL_CLAMP_TO_BORDER
  MirrorClampToEdge = 0x8743 // GL_MIRROR_CLAMP_TO_EDGE
};

enum class TextureFilteringMode : u32
{
  Nearest = 0x2600,               // GL_NEAREST
  Linear = 0x2601,                // GL_LINEAR
  NearestMipmapNearest = 0x2700,  // GL_NEAREST_MIPMAP_NEAREST
  LinearMipmapNearest = 0x2701,   // GL_LINEAR_MIPMAP_NEAREST
  NearestMipmapLinear = 0x2702,   // GL_NEAREST_MIPMAP_LINEAR
  LinearMipmapLinear = 0x2703,    // GL_LINEAR_MIPMAP_LINEAR
};

enum class PixelDataType : u32
{
  UnsignedByte    = 0x1401, // GL_UNSIGNED_BYTE,
  Float           = 0x1406, // GL_FLOAT,
  HalfFloat       = 0x140B, // GL_HALF_FLOAT,
  UnsignedInt24_8 = 0x84FA, // GL_UNSIGNED_INT_24_8,
};

enum class PixelDataFormat : u32
{
  Red             = 0x1903, // GL_RED,
  RG              = 0x8227, // GL_RG,
  RGB             = 0x1907, // GL_RGB,
  RGBA            = 0x1908, // GL_RGBA,
  DepthComponent  = 0x1902, // GL_DEPTH_COMPONENT,
  DepthStencil    = 0x84F9, // GL_DEPTH_STENCIL,
};

// A texture is an OpenGL Object that contains one or more images that all have the same image format. 
// An Image Format describes the way that the images in Textures store their data.
// There are three basic kinds of image formats: color, depth, and depth/stencil. 
// 
// A texture can be used in two ways: it can be the source of a texture access from a Shader, or it can be used as a render target.
// A texture object has the concept of "completeness"; a complete texture object is one which is in a logical state to be used for many operations. 
// Until a texture is complete, it cannot be used in shader sampling or Image Load Store operations.
class Texture 
{
public:
  Texture() : m_id{ 0 } {}
  
  static Texture create_from_file(const std::filesystem::path& path, 
                                  TextureImageFormat image_format = TextureImageFormat::RGBA8,
                                  PixelDataFormat pixel_format = PixelDataFormat::RGBA,
                                  PixelDataType pixel_type = PixelDataType::UnsignedByte,
                                  i32 desired_channels = 4,
                                  TextureWrapMode wrap_s = TextureWrapMode::Repeat,
                                  TextureWrapMode wrap_t = TextureWrapMode::Repeat,
                                  TextureFilteringMode mag_filter = TextureFilteringMode::Linear,
                                  TextureFilteringMode min_filter = TextureFilteringMode::LinearMipmapLinear);
  
  static Texture create_from_memory(const void* buffer,
                                    i32 length,
                                    TextureImageFormat image_format = TextureImageFormat::RGBA8,
                                    PixelDataFormat pixel_format = PixelDataFormat::RGBA,
                                    PixelDataType pixel_type = PixelDataType::UnsignedByte,
                                    i32 desired_channels = 4,
                                    TextureWrapMode wrap_s = TextureWrapMode::Repeat,
                                    TextureWrapMode wrap_t = TextureWrapMode::Repeat,
                                    TextureFilteringMode mag_filter = TextureFilteringMode::Linear,
                                    TextureFilteringMode min_filter = TextureFilteringMode::LinearMipmapLinear);
  
  
  // create texture object
  void create(TextureType type);
  // destroy texture object
  void destroy();
  
  // Specify storage for all levels of a 1D or 2D array texture.
  // Once a texture is specified with this command, the format and dimensions of all levels become immutable.
  // The contents of the image may still be modified.
  void set_storage_tex2D(i32 levels, TextureImageFormat image_format, i32 width, i32 height);
  // Update the contents of a 2D texture subimage.
  void update_content_tex2D(i32 level, 
    i32 xoffset, 
    i32 yoffset, 
    i32 width, 
    i32 height, 
    PixelDataFormat format, 
    PixelDataType type, 
    const void* data) const;
  
  
  // Edge value sampling. Normalized texture coordinates are not limited to values between 0.0 and 1.0. 
  // They can be any floating-point number. When a texture coordinate is not within the [0, 1] range, 
  // a heuristic must be employed to decide what the color value will be.
  void set_wrap_mode(TextureWrapMode wrap_s, TextureWrapMode  wrap_t) const;
  // The GL_CLAMP_TO_BORDER requires a color that edge texels are blended when texture coordinates fall outside of the valid area of the texture.
  // The border color can be provided in floating-point values.
  // The border color provided must use integer/float values consistent with the Image Format for the texture being used.
  void set_border_color(f32 r, f32 g, f32 b, f32 a) const;
  
  // Texture filtering is the process of accessing a particular sample from a texture.
  // There are two cases for filtering: minification and magnification.
  // Magnification means that the area of the fragment in texture space is smaller than a texel,
  // and minification means that the area of the fragment in texture space is larger than a texel.
  // Filtering for these two cases can be set independently.
  //
  // The magnification filter value can be either GL_LINEAR or GL_NEAREST.
  // If GL_NEAREST is used, then the implementation will select the texel nearest the texture coordinate; this is commonly called "point sampling". 
  // If GL_LINEAR is used, the implementation will perform a weighted linear blend between the nearest adjacent samples.
  void set_magnification_filter(TextureFilteringMode filter) const;
  // When doing minification, you can choose to use mipmapping or not. 
  // Using mipmapping means selecting between multiple mipmaps based on the angle and size of the texture relative to the screen. 
  // If you do use mipmapping, you can choose to either select a single mipmap to sample from, or you can sample the two adjacent 
  // mipmaps and linearly blend the resulting values to get the final result. 
  void set_minification_filter(TextureFilteringMode filter) const;
  // Generate mipmaps for a specified texture object
  void generate_mipmaps() const;
  
  // A texture can be bound to one or more locations for rendering, where `unit` is the 0-indexed texture unit you'd like to bind to.
  void bind_texture_unit(u32 unit) const;
  
  // check if texture is valid
  bool is_valid() const;
  
  auto id() const { return m_id; }
private:
  u32 m_id;
  TextureImageFormat m_image_format;
};