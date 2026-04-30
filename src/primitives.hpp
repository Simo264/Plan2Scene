#pragma once

#include <vector>
#include <string>

struct Vec2
{  
  float x, y;
};

struct Segment
{
  // std::string layer;
  Vec2 p1, p2;
};

struct Polyline
{
  // std::string layer;
  std::vector<Vec2> points;
  bool closed;
};