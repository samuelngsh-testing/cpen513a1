// @file:     router.cc
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Router class.

#include <QFile>
#include <QDir>
#include <QQueue>
#include <QDebug>
#include "router.h"

using namespace rt;

// Constructor
Router::Router(const Problem &problem, RouterSettings settings)
  : problem(problem), settings(settings)
{
  records = new RoutingRecords(settings.detail_level, settings.gui_update_level);
}

bool Router::routeSuite(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid,
    SolveCollection *solve_col)
{
  RoutingAlg *alg;
  switch (settings.use_alg) {
    case LeeMoore:
      alg = new LeeMooreAlg();
      break;
    case AStar:
    default:
      alg = new AStarAlg();
      break;
  }

  bool all_succeeded=false;
  records->setSolveCollection(solve_col);

  // convenient alias for pairs of coordinates
  using PinPair = QPair<sp::Coord, sp::Coord>;

  // keep track of which pins have yet to be routed
  QSet<sp::Coord> unrouted_pins;
  // construct a matrix that stores the distances between pins within each pin set
  // TODO delete this if the map version works well enough
  // construct a QMultiMap that uses the distances between pins as keys
  QMultiMap<int, PinPair> map_pin_sets;

  // init the lists above
  // TODO put in separate function
  for (sp::PinSet pin_set : pin_sets) {
    for (int i=0; i<pin_set.size(); i++) {
      for (int j=i; j<pin_set.size(); j++) {
        if (i==j) {
          unrouted_pins.insert(pin_set[i]);
        } else {
          int md = pin_set[i].manhattanDistance(pin_set[j]);
          map_pin_sets.insert(md, qMakePair(pin_set[i], pin_set[j]));
        }
      }
    }
  }

  int attempts_left = 20;
  int difficult_boost_thresh = 2; // boost the order of the difficult pin after failing this many times
  bool all_done = false;
  QQueue<PinPair> priority_routes;
  QSet<sp::Coord> failed_pins;
  QList<PinPair> difficult_pairs;
  QMap<PinPair, int> difficult_pair_failure_count;
  sp::Grid *cell_grid_cp = new sp::Grid();  // keep a copy of the cell grid
  cell_grid_cp->copyState(cell_grid);
  auto map_pin_sets_cp = map_pin_sets;
  // TODO remove QQueue<PinPair> successful_routes;
  // TODO following implementation only contains one attempt pass. Proper 
  // implementation needs multiple passes
  while (!all_done && attempts_left > 0 && !map_pin_sets.isEmpty()) {
    PinPair pin_pair;
    if (!priority_routes.isEmpty()) {
      pin_pair = priority_routes.dequeue();
    } else {
      pin_pair = map_pin_sets.take(map_pin_sets.firstKey());
    }
    sp::Coord source_coord, sink_coord;
    if (unrouted_pins.contains(pin_pair.first)) {
      source_coord = pin_pair.first;
      sink_coord = pin_pair.second;
    } else if (unrouted_pins.contains(pin_pair.second)) {
      source_coord = pin_pair.second;
      sink_coord = pin_pair.first;
    } else {
      // both pins have already been routed
      continue;
    }
    bool success;
    if (cell_grid->routeExistsBetweenPins(source_coord, sink_coord)) {
      //qDebug() << tr("Has existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      success = true;
    } else {
      //qDebug() << tr("No existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      //success = routeForId(AStar, {pin_pair.first, pin_pair.second}, cell_grid, solve_col);
      success = alg->findRoute(source_coord, sink_coord, cell_grid, 
          settings.routed_cells_lower_cost, records);
    }
    if (!success) {
      if (!failed_pins.contains(source_coord) && !failed_pins.contains(sink_coord)) {
        difficult_pair_failure_count[pin_pair] += 1;
        if (!difficult_pairs.contains(pin_pair)) {
          // every time a new failure emerges, put them as top priority for the next round
          difficult_pairs.prepend(pin_pair);
        } else if (difficult_pair_failure_count[pin_pair] == difficult_boost_thresh) {
          difficult_pairs.removeOne(pin_pair);
          difficult_pairs.prepend(pin_pair);
          difficult_pair_failure_count[pin_pair] = 0;
        }
        failed_pins.insert(source_coord);
        failed_pins.insert(sink_coord);
      }
    }
    // TODO build in some way to consider give up conditions
    if (map_pin_sets.isEmpty() && failed_pins.isEmpty()) {
      all_done = true;
      all_succeeded = true;
    } else if (map_pin_sets.isEmpty() && !failed_pins.isEmpty()) {
      for (auto difficult_pair : difficult_pairs) {
        priority_routes.enqueue(difficult_pair);
      }
      cell_grid->copyState(cell_grid_cp);
      map_pin_sets = map_pin_sets_cp;
      failed_pins.clear();
      attempts_left--;
      qDebug() << tr("\nNo solution found, attempts left: %1").arg(attempts_left);
    }
  }

  // verify that all pins have indeed been routed (sanity check)
  if (all_succeeded) {
    bool cell_grid_routed = cell_grid->allPinsRouted();
    if (all_succeeded != cell_grid_routed) {
      qWarning() << tr("Route success state differs between program and Grid's "
          "return. Program thinks %1, Grid thinks %2.").arg(all_succeeded)
        .arg(cell_grid_routed);
    } else {
      qDebug() << tr("ALL ROUTES COMPLETED SUCCESSFULLY.");
    }
  }

  delete cell_grid_cp;
  delete alg;

  return all_succeeded;
}

