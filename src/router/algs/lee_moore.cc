// @file:     lee_moore.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Lee-Moore algorithm.

#include <QDebug>
#include "lee_moore.h"

using namespace rt;

RouteResult LeeMooreAlg::findRoute(const sp::Coord &source_coord, 
    const sp::Coord &sink_coord, sp::Grid *grid, bool t_routed_cells_lower_cost, 
    bool clear_working_values, bool, QList<sp::Connection*> *,
    RoutingRecords *record_keeper)
{
  routed_cells_lower_cost = t_routed_cells_lower_cost;
  sp::Cell *source_cell = grid->cellAt(source_coord);
  int pin_set_id = source_cell->pinSetId();
  sp::Coord termination;
  QList<sp::Coord> route;

  // run Lee Moore forward pass
  qDebug() << QObject::tr("Running Lee-Moore from %1 to %2")
    .arg(source_coord.str()).arg(sink_coord.str());
  bool success = runLeeMoore(source_coord, sink_coord, grid, pin_set_id, 
      termination, route, record_keeper);

  if (success) {
    runBacktrace(termination, source_coord, sink_coord, grid, pin_set_id, route,
        record_keeper);
  } else {
    qDebug() << QObject::tr("Failed to run Lee-Moore on pin set %1").arg(pin_set_id);
  }
  if (record_keeper != nullptr) {
    record_keeper->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
  }

  // clear all working values
  if (clear_working_values) {
    grid->clearWorkingValues();
  }
  /*
     TODO remove
  if (record_keeper != nullptr) {
    record_keeper->logCellGrid(grid, LogResultsOnly, VisualizeResultsOnly);
  }
  */

  RouteResult result;
  result.route_coords = route;
  result.requires_rip = false;  // ripping support not implemented in Lee-Moore

  return result;
}

QList<sp::Coord> LeeMooreAlg::markNeighbors(const sp::Coord &coord, sp::Grid *grid,
    int pin_set_id, bool &marked) const
{
  marked=false;
  QList<sp::Coord> neighbors = grid->neighborCoordsOf(coord);
  // mark each neighbor if eligible
  for (const sp::Coord &neighbor : neighbors) {
    sp::Cell *cell = grid->cellAt(neighbor);
    if ((cell->getType() == sp::BlankCell || cell->pinSetId() == pin_set_id)
        && (cell->workingValue() < 0)) {
      // eligible neighbor found
      int cost;
      if (routed_cells_lower_cost && cell->pinSetId() == pin_set_id) {
        cost = 40;
      } else {
        cost = 100;
      }
      cell->setWorkingValue(grid->cellAt(coord)->workingValue()+cost);
      marked=true;
    } else {
      // discard ineligible neighbor
      neighbors.removeOne(neighbor);
    }
  }
  return neighbors;
}

bool LeeMooreAlg::runLeeMoore(const sp::Coord &source_coord,
    const sp::Coord &sink_coord, sp::Grid *grid, int pin_set_id, 
    sp::Coord &termination, QList<sp::Coord> &term_to_sink_route, 
    RoutingRecords *record_keeper) const
{
  bool marked;
  // add source to evaluation list
  QList<sp::Coord> neighbors({source_coord});
  grid->cellAt(source_coord)->setWorkingValue(0);
  // loop through neighbors until sink or eligible route found
  while (!neighbors.isEmpty()) {
    sp::Coord base_coord = neighbors.takeFirst();
    sp::Cell *base_cell = grid->cellAt(base_coord);
    if (base_cell->pinSetId() == pin_set_id 
        && grid->routeExistsBetweenPins(base_coord, sink_coord, &term_to_sink_route)) {
      // return if a connection has been made to the sink or a routed wire that
      // leads to the sink
      termination = base_coord;
      term_to_sink_route.append(termination);
      return true;
    }
    // keep marking neighbors
    neighbors.append(markNeighbors(base_coord, grid, pin_set_id, marked));
    if (marked && record_keeper != nullptr) {
      record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
    }
  }
  // reaching this point means that no solution was found
  return false;
}

void LeeMooreAlg::runBacktrace(const sp::Coord &curr_coord, 
    const sp::Coord &source_coord, const sp::Coord &sink_coord, sp::Grid *grid,
    int pin_set_id, QList<sp::Coord> &route, RoutingRecords *record_keeper) const
{
  if (curr_coord == source_coord) {
    // backtrace complete
    return;
  } else {
    int curr_working_val = grid->cellAt(curr_coord)->workingValue();
    for (sp::Cell *nc : grid->neighborsOf(curr_coord)) {
      if (nc->workingValue() == 0) {
        // back tracing complete
        return;
      } else if (nc->workingValue() >= 0 && nc->workingValue() < curr_working_val) {
        /* TODO remove
        // update cell type for chosen routed cells
        nc->setType(sp::RoutedCell);
        nc->setPinSetId(pin_set_id);
        if (record_keeper != nullptr) {
          record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
        }
        */
        route.append(nc->getCoord());
        // recurse until source reached
        runBacktrace(nc->getCoord(), source_coord, sink_coord, grid, pin_set_id,
            route, record_keeper);
        break;
      }
    }
  }
}
