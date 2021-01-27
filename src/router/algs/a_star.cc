// @file:     a_star.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     A* algorithm using the Alg base class.

#include <QVariant>
#include <QDebug>
#include "a_star.h"

using namespace rt;

RouteResult AStarAlg::findRoute(const sp::Coord &source_coord, 
    const sp::Coord &sink_coord, sp::Grid *grid, bool t_routed_cells_lower_cost,
    bool clear_working_values, bool t_attempt_rip,
   QList<sp::Connection*> *t_rip_blacklist, RoutingRecords *record_keeper)
{
  routed_cells_lower_cost = t_routed_cells_lower_cost;
  rip_blacklist = t_rip_blacklist;
  attempt_rip = t_attempt_rip;  // attempt rip if a rip_cand pointer is given
  RouteResult result;


  sp::Coord termination;  // valid termination (sink or routed cell)
  QList<sp::Coord> route;
  int pin_set_id = grid->cellAt(source_coord)->pinSetId();
  bool route_requires_rip;
  bool success = runAStar(source_coord, sink_coord, grid, pin_set_id, 
      termination, route, route_requires_rip, record_keeper);
  result.requires_rip = route_requires_rip;

  if (record_keeper != nullptr) {
    record_keeper->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
  }

  if (success) {
    runBacktrace(termination, source_coord, grid, pin_set_id, route, record_keeper);
    result.route_coords = route;
  }

  // clear all working values
  if (clear_working_values) {
    grid->clearWorkingValues();
  }

  return result;
}

QMultiMap<QPair<int,int>,sp::Coord> AStarAlg::markNeighbors(const sp::Coord &coord,
    const sp::Coord &source_coord, const sp::Coord &sink_coord, sp::Grid *grid,
    int pin_set_id, bool &marked, sp::Coord &termination,
    QList<sp::Coord> &term_to_sink_route, 
    QMultiMap<std::tuple<int,int,int>, sp::Coord> &rip_neighbors) const
{
  marked = false;
  termination = sp::Coord();
  QMultiMap<QPair<int,int>,sp::Coord> expl_map;
  QList<sp::Coord> neighbors = grid->neighborCoordsOf(coord);
  // mark each neighbor if eligible
  for (const sp::Coord &neighbor : neighbors) {
    sp::Cell *nc = grid->cellAt(neighbor);
    QList<sp::Connection*> coord_conns = grid->connMap()->values(neighbor);
    // is candidate without rip:
    bool is_cand_wo_rip = nc->getType() == sp::BlankCell || nc->pinSetId() == pin_set_id;
    // is candidate with rip if connection to be ripped is not on blacklist:
    bool is_cand_w_rip = nc->getType() == sp::RoutedCell && nc->pinSetId() != pin_set_id
      && std::all_of(coord_conns.begin(), coord_conns.end(),
          [this](sp::Connection* conn){return !rip_blacklist->contains(conn);});
    if (is_cand_wo_rip || (is_cand_w_rip && attempt_rip)) {
      // eligible neighbor found
      int d_from_source = grid->cellAt(coord)->extraProps()["d_from_source"].toInt();
      if (routed_cells_lower_cost && nc->getType() == sp::RoutedCell && nc->pinSetId() == pin_set_id) {
        d_from_source += 40;
      } else {
        d_from_source += 100;
      }
      int ripped_conns = grid->cellAt(coord)->extraProps()["ripped_conns"].toInt();
      if (is_cand_w_rip) {
        ripped_conns += grid->connMap()->count(neighbor);
        d_from_source += 50000;
      }
      int md_sink = 100*neighbor.manhattanDistance(sink_coord);
      int priority = neighbor.manhattanDistance(sink_coord);
      int working_val = d_from_source + md_sink;
      bool update_cell = is_cand_wo_rip && (nc->workingValue() < 0 || nc->workingValue() > working_val);
      update_cell |= is_cand_w_rip && 
        (nc->extraProps()["ripped_conns"] <= 0 || nc->extraProps()["ripped_conns"] > ripped_conns);
      if (update_cell) {
        // update values in the newly traversed neighbor
        nc->setWorkingValue(working_val);
        QVariant from_coord_v;
        from_coord_v.setValue(coord);
        QVariant source_coord_v, sink_coord_v;
        source_coord_v.setValue(source_coord);
        sink_coord_v.setValue(sink_coord);
        nc->extraProps()["from_coord"] = from_coord_v;
        nc->extraProps()["d_from_source"] = d_from_source;
        nc->extraProps()["ripped_conns"] = ripped_conns;
        nc->extraProps()["source_coord"] = source_coord_v;
        nc->extraProps()["sink_coord"] = sink_coord_v;
        if (ripped_conns == 0) {
          expl_map.insert(qMakePair(working_val, priority), neighbor);
        } else {
          auto key = std::make_tuple(ripped_conns, d_from_source, priority);
          rip_neighbors.insert(key, neighbor);
        }
        // bookeeping
        marked = true;
        bool is_elig_rcell = !is_cand_w_rip && (nc->getType() == sp::RoutedCell 
            && grid->routeExistsBetweenPins(neighbor, sink_coord, &term_to_sink_route));
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
    QList<sp::Coord> &term_to_sink_route, bool &route_requires_rip,
    RoutingRecords *record_keeper) const
{
  bool marked;
  // keep a map of neighbors to be explored -- first key is the A* score and 
  // second key the priority determined by distance to sink
  QMultiMap<QPair<int,int>,sp::Coord> expl_map;
  bool exploring_rip_solutions=false;
  // keep a map of neighbors that can be accessed if ripping is allowed. See the
  // comment of the markNeighbors function for what the key tuple represents.
  QMultiMap<std::tuple<int,int,int>,sp::Coord> rip_neighbors;
  // add the source coord as the first element to look at
  int md = source_coord.manhattanDistance(sink_coord);
  expl_map.insert(qMakePair(md,0), source_coord);
  grid->cellAt(source_coord)->extraProps()["d_from_source"] = 0;
  grid->cellAt(source_coord)->extraProps()["ripped_conns"] = 0;
  grid->cellAt(source_coord)->setWorkingValue(md*100);
  // loop through neighbors list until sink or eligible routed cell found
  while ((!expl_map.isEmpty()) 
      || (attempt_rip && exploring_rip_solutions && !rip_neighbors.isEmpty())) {
    // take the coordinate with the minimum working value
    sp::Coord coord_mwv;
    if (!exploring_rip_solutions) {
      coord_mwv = expl_map.take(expl_map.firstKey());
    } else {
      coord_mwv = rip_neighbors.take(rip_neighbors.firstKey());
    }
    // insert newly found neighbors to the exploration map
    expl_map.unite(markNeighbors(coord_mwv, source_coord, sink_coord, grid, 
          pin_set_id, marked, termination, term_to_sink_route, rip_neighbors));
    if (marked && record_keeper != nullptr) {
      record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
    }
    if (!termination.isBlank()) {
      // markNeighbors would already have filled in most of the 
      // term_to_sink_route so just add the termination cell
      term_to_sink_route.append(termination);
      route_requires_rip = exploring_rip_solutions;
      return true;
    } else {
      if (expl_map.isEmpty()) {
        exploring_rip_solutions = true;
      }
    }
  }
  // reaching this point means that no solution was found in the loop
  return false;
}

void AStarAlg::runBacktrace(const sp::Coord &curr_coord, const sp::Coord &source_coord,
    sp::Grid *grid, int pin_set_id, QList<sp::Coord> &route, 
    RoutingRecords *record_keeper) const
{
  if (curr_coord == source_coord) {
    return;
  } else {
    QVariant from_coord_v = grid->cellAt(curr_coord)->extraProps()["from_coord"];
    sp::Coord from_coord = from_coord_v.value<sp::Coord>();
    route.append(from_coord);
    
    runBacktrace(from_coord, source_coord, grid, pin_set_id, route, record_keeper);
  }
}
