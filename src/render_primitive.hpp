#pragma once

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/PhongGL.h>

class PrimitivesExample: public Magnum::Platform::Application
{
  public:
    explicit PrimitivesExample(const Arguments& arguments);

  private:
    void drawEvent() override;
    void pointerMoveEvent(PointerMoveEvent& event) override;
    
    Magnum::GL::Mesh _mesh;
    Magnum::Shaders::PhongGL _shader;
    Magnum::Matrix4 _transformation, _projection;
    Magnum::Color3 _color;
};
