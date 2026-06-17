#pragma once

#include "../types.hpp"

#include <libdxfrw.h>
#include <drw_interface.h>

class DRWParser : public DRW_Interface 
{
public: 
  virtual void addHeader(const DRW_Header* data) override;
  virtual void addLayer(const DRW_Layer& data) override;
  virtual void addLine(const DRW_Line& data) override;
  virtual void addPolyline(const DRW_Polyline& data) override;
  virtual void addLWPolyline(const DRW_LWPolyline& data) override;
  virtual void addArc(const DRW_Arc& data) override;
  virtual void addInsert([[maybe_unused]]const DRW_Insert& data) override;
  virtual void addBlock([[maybe_unused]]const DRW_Block& data) override;
  virtual void endBlock() override;
  
  virtual void addHatch([[maybe_unused]]const DRW_Hatch* data) override {}
  virtual void addCircle([[maybe_unused]]const DRW_Circle& data) override {}
  virtual void addSolid([[maybe_unused]]const DRW_Solid& data) override {}
  virtual void addSpline([[maybe_unused]]const DRW_Spline* data) override {}
  virtual void addPoint([[maybe_unused]]const DRW_Point& data) override {}
  virtual void addEllipse([[maybe_unused]]const DRW_Ellipse& data) override {}

  virtual void add3dFace([[maybe_unused]] const DRW_3Dface& data) override {}
  virtual void addText([[maybe_unused]] const DRW_Text& data) override {}
  virtual void addMText([[maybe_unused]] const DRW_MText& data) override {}
  virtual void addDimAlign([[maybe_unused]] const DRW_DimAligned* data) override {}
  virtual void addDimLinear([[maybe_unused]] const DRW_DimLinear* data) override {}
  virtual void addDimRadial([[maybe_unused]] const DRW_DimRadial* data) override {}
  virtual void addDimDiametric([[maybe_unused]] const DRW_DimDiametric* data) override {}
  virtual void addDimAngular([[maybe_unused]] const DRW_DimAngular* data) override {}
  virtual void addDimAngular3P([[maybe_unused]] const DRW_DimAngular3p* data) override {}
  virtual void addDimOrdinate([[maybe_unused]] const DRW_DimOrdinate* data) override {}
  virtual void addLeader([[maybe_unused]] const DRW_Leader* data) override {}
  virtual void addViewport([[maybe_unused]] const DRW_Viewport& data) override {}
  virtual void addImage([[maybe_unused]] const DRW_Image* data) override {}
  virtual void linkImage([[maybe_unused]] const DRW_ImageDef* data) override {}
  
  virtual void addTextStyle([[maybe_unused]] const DRW_Textstyle& data) override {}
  virtual void addLType([[maybe_unused]] const DRW_LType& data) override {}
  virtual void addDimStyle([[maybe_unused]] const DRW_Dimstyle& data) override {}
  virtual void addVport([[maybe_unused]] const DRW_Vport& data) override {}
  virtual void addComment([[maybe_unused]] const char* comment) override {}
  virtual void setBlock([[maybe_unused]] const int handle) override {}
  virtual void addAppId([[maybe_unused]] const DRW_AppId& data) override {}
  virtual void addRay([[maybe_unused]] const DRW_Ray& data) override {}
  virtual void addXline([[maybe_unused]] const DRW_Xline& data) override {}
  virtual void addKnot([[maybe_unused]] const DRW_Entity& data) override {}
  virtual void addTrace([[maybe_unused]] const DRW_Trace& data) override {}
  virtual void addPlotSettings([[maybe_unused]] const DRW_PlotSettings* data) override {}

  virtual void writeHeader([[maybe_unused]] DRW_Header& data) override {}
  virtual void writeBlocks() override {}
  virtual void writeBlockRecords() override {}
  virtual void writeEntities() override {}
  virtual void writeLTypes() override {}
  virtual void writeLayers() override {}
  virtual void writeTextstyles() override {}
  virtual void writeVports() override {}
  virtual void writeDimstyles() override {}
  virtual void writeObjects() override {}
  virtual void writeAppId() override {}

  std::vector<Segment> walls;
  std::vector<Segment> doors;
  std::vector<Segment> windows;
  f32 unit_scale = 0.0f;
};