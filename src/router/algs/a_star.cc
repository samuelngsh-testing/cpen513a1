// @file:     a_star.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     A* algorithm using the Alg base class.

#include <QVariant>
#include "a_star.h"

using namespace rt;

bool AStarAlg::findRoute(const sp::Coord &source_coord, 
    const sp::Coord &sink_coord, sp::Grid *grid, bool t_routed_cells_lower_cost,
    RoutingRecords *record_keeper)
{
  routed_cells_lower_cost = t_routed_cells_lower_cost;

  if (record_keeper != nullptr) {
    record_keeper->newSolveSteps();
  }

  sp::Coord termination;  // valid termination (sink or routed cell)
  int pin_set_id = grid->cellAt(source_coord)->pinSetId();
  bool success = runAStar(source_coord, sink_coord, grid, pin_set_id, 
      termination, record_keeper);

  if (success) {
    runBacktrace(termination, source_coord, grid, pin_set_id, record_keeper);
  }
  if (record_keeper != nullptr) {
    record_keeper->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
  }

  // clear all working values
  grid->clearWorkingValues();
  if (record_keeper != nullptr) {
    record_keeper->logCellGrid(grid, LogResultsOnly, VisualizeResultsOnly);
  }

  return success;
}

QMultiMap<QPair<int,int>,sp::Coord> AStarAlg::markNeighbors(const sp::Coord &coord,
    const sp::Coord &source_coord, const sp::Coord &sink_coord, sp::Grid *grid,
    int pin_set_id, bool &marked, sp::Coord &termination) const
{
  marked = false;
  termination = sp::Coord();
  QMultiMap<QPair<int,int>,sp::Coord> expl_map;
  QList<sp::Coord> neighbors = grid->neighborCoordsOf(coord);
  // mark each neighbor if eligible
  for (const sp::Coord &neighbor : neighbors) {
    sp::Cell *nc = grid->cellAt(neighbor);
    if (nc->getType() == sp::BlankCell || nc->pinSetId() == pin_set_id) {
      // eligible neighbor found
      int d_from_source = grid->cellAt(coord)->extraProps()["d_from_source"].toInt();
      if (routed_cells_lower_cost && nc->getType() == sp::RoutedCell && nc->pinSetId() == pin_set_id) {
        d_from_source += 40;
      } else {
        d_from_source += 100;
      }
      int md_sink = 100*neighbor.manhattanDistance(sink_coord);
      int priority = neighbor.manhattanDistance(sink_coord);
      int working_val = d_from_source + md_sink;
      if (nc->workingValue() < 0 || nc->workingValue() > working_val) {
        // update values in the newly traversed neighbor
        nc->setWorkingValue(working_val);
        QVariant from_coord_v;
        from_coord_v.setValue(coord);
        QVariant source_coord_v, sink_coord_v;
        source_coord_v.setValue(source_coord);
        sink_coord_v.setValue(sink_coord);
        nc->extraProps()["from_coord"] = from_coord_v;
        nc->extraProps()["d_from_source"] = d_from_source;
        nc->extraProps()["source_coord"] = source_coord_v;
        nc->extraProps()["sink_coord"] = sink_coord_v;
        expl_map.insert(qMakePair(working_val, priority), neighbor);
        // bookeeping
        marked = true;
        bool is_elig_rcell = (nc->getType() == sp::RoutedCell 
            && grid->routeExistsBetweenPins(neighbor, sink_coord));
        if (neighbor == sink_coord || is_elig_rcell) {
          termination = neighbor;
        }
      }
    }
  }
  return expl_map;
}

bool AStarAlg::runAStar(const sp::Coord &source_coord, const sp::Coord &sink_coord,
    sp::Grid *grid, int pin_set_id, sp::Coord &termination, 
    RoutingRecords *record_keeper) const
{
  bool marked;
  // keep a map of neighbors to be explored -- first key is the A* score and 
  // second key the priority determined by distance to sink
  QMultiMap<QPair<int,int>,sp::Coord> expl_map;
  // add the source coord as the first element to look at
  int md = source_coord.manhattanDistance(sink_coord);
  expl_map.insert(qMakePair(md,0), source_coord);
  grid->cellAt(source_coord)->extraProps()["d_from_source"] = 0;
  grid->cellAt(source_coord)->setWorkingValue(md*100);
  // loop through neighbors list until sink or eligible routed cell found
  while (!expl_map.isEmpty()) {
    // take the coordinate with the minimum working value
    sp::Coord coord_mwv = expl_map.take(expl_map.firstKey());
    // insert newly found neighbors to the exploration map
    expl_map.unite(markNeighbors(coord_mwv, source_coord, sink_coord, grid, 
          pin_set_id, marked, termination));
    if (marked && record_keeper != nullptr) {
      record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
    }
    if (!termination.isBlank()) {
      return true;
    }
  }
  // reaching this point means that no solution was found in the loop
  return false;
}

void AStarAlg::runBacktrace(const sp::Coord &curr_coord, const sp::Coord &source_coord,
    sp::Grid *grid, int pin_set_id, RoutingRecords *record_keeper) const
{
  if (curr_coord == source_coord) {
    return;
  } else {
    QVariant from_coord_v = grid->cellAt(curr_coord)->extraProps()["from_coord"];
    sp::Coord from_coord = from_coord_v.value<sp::Coord>();
    sp::Cell *from_cell = grid->cellAt(from_coord);
    if (from_cell->getType() != sp::PinCell) {
      from_cell->setType(sp::RoutedCell);
      from_cell->setPinSetId(pin_set_id);
    }
    if (grid->routeExistsBetweenPins(from_coord, source_coord)) {
      // backtracing ends early because connect is already made
      return;
    }
    if (record_keeper != nullptr) {
      record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
    }
    runBacktrace(from_coord, source_coord, grid, pin_set_id, record_keeper);
  }
}
