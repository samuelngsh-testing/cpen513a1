// @file:     route_graph.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Implementation of routing graphs.

#include "route_graph.h"

using namespace rt;

void RCell::setBlankCell()
{
  setType(sp::BlankCell);
  g_edge=nullptr;
  g_node=nullptr;
}

void RCell::setRoutedCell(int pin_set_id, GEdge *e, GNode *n)
{
  setPinSetId(pin_set_id);
  if (e != nullptr && n != nullptr) {
    qFatal("A GCell cannot be simultanously a GEdge and a GNode.");
  }
  g_edge = e;
  g_node = n;
}
