#include "drw_parser.hpp"

#include <print>

void DRWParser::addLine(const DRW_Line& data)
{
  std::println("Line from ({:.2f}, {:.2f}) to ({:.2f}, {:.2f})", data.basePoint.x, data.basePoint.y, data.secPoint.x, data.secPoint.y);
}

void DRWParser::addCircle(const DRW_Circle& data)
{
  std::println("Circle at ({:.2f}, {:.2f}) radius {:.2f}", data.basePoint.x, data.basePoint.y, data.radious);
}