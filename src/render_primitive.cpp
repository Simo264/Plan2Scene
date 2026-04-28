#include "render_primitive.hpp"

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Math/Angle.h>


PrimitivesExample::PrimitivesExample(const Arguments& arguments) 
  : Magnum::Platform::Application{arguments, Configuration{}.setTitle("Magnum Primitives Example")}
{
  using namespace Magnum::Math::Literals;

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  
  _mesh = Magnum::MeshTools::compile(Magnum::Primitives::cubeSolid());
  _transformation = Magnum::Matrix4::rotationX(30.0_degf)*Magnum::Matrix4::rotationY(40.0_degf);
  _projection = Magnum::Matrix4::perspectiveProjection(35.0_degf, Magnum::Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f) * Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-10.0f)); 
  _color = Magnum::Color3::fromHsv({35.0_degf, 1.0f, 1.0f});
  _shader = Magnum::Shaders::PhongGL{};
}


void PrimitivesExample::drawEvent()
{
  Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth);
  _shader.setLightPositions({{1.4f, 1.0f, 0.75f, 0.0f}})
         .setDiffuseColor(_color)
         .setAmbientColor(Magnum::Color3::fromHsv({_color.hue(), 1.0f, 0.3f}))
         .setTransformationMatrix(_transformation)
         .setNormalMatrix(_transformation.normalMatrix())
         .setProjectionMatrix(_projection)
         .draw(_mesh);
  swapBuffers();
}

void PrimitivesExample::pointerMoveEvent(PointerMoveEvent& event) 
{
  if(!event.isPrimary() || !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
    return;

   Magnum::Vector2 delta = 3.0f*Magnum::Vector2{event.relativePosition()}/Magnum::Vector2{windowSize()};

   _transformation = Magnum::Matrix4::rotationX(Magnum::Rad{delta.y()})* _transformation* Magnum::Matrix4::rotationY(Magnum::Rad{delta.x()});

   event.setAccepted();
   redraw();
}