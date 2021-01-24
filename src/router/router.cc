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
  records = new RoutingRecords(settings.log_level, settings.gui_update_level);
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

  // keep track of which pins have yet to be routed
  QSet<sp::Coord> unrouted_pins;
  // construct a matrix that stores the distances between pins within each pin set
  // TODO delete this if the map version works well enough
  // construct a QMultiMap that uses the distances between pins as keys
  QMultiMap<int, sp::PinPair> map_pin_sets;

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
  QQueue<sp::PinPair> priority_routes;
  QSet<sp::Coord> failed_pins;
  QList<sp::PinPair> difficult_pairs;
  QMap<sp::PinPair, int> difficult_pair_failure_count;
  sp::Grid *cell_grid_cp = new sp::Grid();  // keep a copy of the cell grid
  cell_grid_cp->copyState(cell_grid);
  auto map_pin_sets_cp = map_pin_sets;
  // TODO remove QQueue<sp::PinPair> successful_routes;
  // TODO following implementation only contains one attempt pass. Proper 
  // implementation needs multiple passes
  while (!all_done && attempts_left > 0 && !map_pin_sets.isEmpty()) {
    sp::PinPair pin_pair;
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
    QList<sp::Coord> route;
    bool success;
    if (cell_grid->routeExistsBetweenPins(source_coord, sink_coord, &route)) {
      // TODO add this to Connection tracking
      //qDebug() << tr("Has existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      success = true;
      createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(),
          cell_grid, records);
    } else {
      //qDebug() << tr("No existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      //success = routeForId(AStar, {pin_pair.first, pin_pair.second}, cell_grid, solve_col);
      QList<sp::Connection*> rip_cand;
      QList<sp::Connection*> rip_blacklist;
      success = false;
      /* TODO fix after rip and reroute implemented
      route = alg->findRoute(source_coord, sink_coord, cell_grid, 
          settings.routed_cells_lower_cost, false, &rip_cand, &rip_blacklist, records);
          */
      if (!route.isEmpty()) {
        success = true;
        createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(), 
            cell_grid, records);
        cell_grid->clearWorkingValues();
        if (records != nullptr) {
          records->logCellGrid(cell_grid, LogResultsOnly, VisualizeResultsOnly);
        }
      }
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
      qDebug() << tr("****No solution found, attempts left: %1****").arg(attempts_left);
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

bool Router::routeSuiteRipReroute(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid,
    bool *soft_halt, SolveCollection *solve_col)
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

  // keep track of which pins have yet to be routed
  QSet<sp::Coord> unrouted_pins;
  // construct a matrix that stores the distances between pins within each pin set
  // TODO delete this if the map version works well enough
  // construct a QMultiMap that uses the distances between pins as keys
  QMultiMap<int, sp::PinPair> map_pin_sets;

  bool rip_and_reroute = settings.rip_and_reroute;
  bool net_reordering = settings.net_reordering;

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

  int attempts_left = (net_reordering) ? settings.max_rerun_count : 1;
  int difficult_boost_thresh = 2; // boost the order of the difficult pin after failing this many times
  bool all_done = false;
  QQueue<sp::PinPair> priority_routes;
  QSet<sp::Coord> failed_pins;
  QList<sp::PinPair> difficult_pairs;
  QMap<sp::PinPair, int> difficult_pair_failure_count;
  sp::Grid *cell_grid_cp = new sp::Grid();  // keep a copy of the cell grid
  cell_grid_cp->copyState(cell_grid);
  auto map_pin_sets_cp = map_pin_sets;
  if (records != nullptr) {
    records->newSolveSteps();
  }
  // TODO remove QQueue<sp::PinPair> successful_routes;
  // TODO following implementation only contains one attempt pass. Proper 
  // implementation needs multiple passes
  while (!(*soft_halt) && !all_done && attempts_left > 0 && !map_pin_sets.isEmpty()) {
    sp::PinPair pin_pair;
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
    QList<sp::Coord> route;
    bool success;
    if (cell_grid->routeExistsBetweenPins(source_coord, sink_coord, &route)) {
      // TODO add this to Connection tracking
      //qDebug() << tr("Has existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      success = true;
      createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(),
          cell_grid, records);
    } else {
      //qDebug() << tr("No existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      //success = routeForId(AStar, {pin_pair.first, pin_pair.second}, cell_grid, solve_col);
      success = false;
      QList<sp::Connection*> rip_cand;
      QList<sp::Connection*> rip_blacklist;
      RouteResult result;
      result = alg->findRoute(source_coord, sink_coord, cell_grid, 
          settings.routed_cells_lower_cost, false, rip_and_reroute, &rip_blacklist, records);
      route = result.route_coords;
      if (!route.isEmpty()) {
        if (!result.requires_rip) {
          // straightforward result that doesn't require ripping
          success = true;
          createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(), 
              cell_grid, records);
          cell_grid->clearWorkingValues();
          if (records != nullptr) {
            records->logCellGrid(cell_grid, LogResultsOnly, VisualizeResultsOnly);
          }
        } else if (rip_and_reroute) {
          // attempt rip and reroute
          // save the cell grid before doing anything
          cell_grid->clearWorkingValues();
          sp::Grid grid_pre_rip;
          grid_pre_rip.copyState(cell_grid);

          // get the connections that need to be ripped to make this successful
          QList<sp::PinPair> pairs_to_reroute;
          QSet<sp::Connection*> conns = existingConnections(route, cell_grid, (*cell_grid)(source_coord)->pinSetId());
          qDebug() << tr("Attempting rip and reroute with %1 routes to rip").arg(conns.count());
          for (sp::Connection *conn : conns) {
            pairs_to_reroute.append(conn->pinPair());
            ripConnection(conn, cell_grid, records);
            delete conn;
            if (records != nullptr) {
              records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
            }
          }

          // create the new connection
          createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(),
              cell_grid, records);

          // reroute the ripped connections
          bool all_rerouted = true;
          for (const sp::PinPair &reroute_pair : pairs_to_reroute) {
            RouteResult reroute_result;
            reroute_result = alg->findRoute(reroute_pair.first, reroute_pair.second, 
                cell_grid, settings.routed_cells_lower_cost, false, false,
                &rip_blacklist, records);
            if (!reroute_result.route_coords.isEmpty()) {
              createConnection(reroute_pair, reroute_result.route_coords,
                  (*cell_grid)(reroute_pair.first)->pinSetId(), cell_grid,
                  records);
              cell_grid->clearWorkingValues();
              if (records != nullptr) {
                records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
              }
            } else {
              cell_grid->clearWorkingValues();
              qDebug() << "Rip and reroute failed because one of the ripped "
                  "routes cannot be rerouted.";
              all_rerouted = false;
              break;
            }
          }

          // revert if not all ripped connections can be rerouted
          if (!all_rerouted) {
            qDebug() << "Reverting to state prior to rerouting";
            cell_grid->copyState(&grid_pre_rip);
            if (records != nullptr) {
              records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
            }
          } else {
            success = true;
          }
        }
      } else {
        cell_grid->clearWorkingValues();
        if (records != nullptr) {
          records->logCellGrid(cell_grid, LogResultsOnly, VisualizeResultsOnly);
        }
      }
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
      qDebug() << tr("****No solution found, attempts left: %1****").arg(attempts_left);
      if (attempts_left > 0 && records != nullptr) {
        records->newSolveSteps();
      }
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

sp::Connection *Router::createConnection(const sp::PinPair &pin_pair,
    const QList<sp::Coord> &route, int pin_set_id, sp::Grid *grid,
    RoutingRecords *record_keeper)
{
  sp::Connection *conn = new sp::Connection(pin_pair, route, pin_set_id);
  for (const sp::Coord &coord : route) {
    // add this to connectivity track map
    grid->connMap()->insert(coord, conn);
    // update cell properties
    sp::Cell *cell = grid->cellAt(coord);
    if (cell->getType() == sp::BlankCell) {
      cell->setType(sp::RoutedCell);
      cell->setPinSetId(pin_set_id);
      if (record_keeper != nullptr) {
        record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
      }
    }
  }
  return conn;
}

QSet<sp::Connection*> Router::existingConnections(const QList<sp::Coord> &coords,
    sp::Grid *grid, int ignore_pin_id)
{
  QSet<sp::Connection*> conns;
  for (const sp::Coord &coord : coords) {
    for (sp::Connection *conn : grid->connMap()->values(coord)) {
      if (conn->pinSetId() != ignore_pin_id) {
        conns.insert(conn);
      }
    }
  }
  return conns;
}

void Router::ripConnection(sp::Connection *conn, sp::Grid *grid,
    RoutingRecords *record_keeper)
{
  auto routed_cells = conn->routedCells();
  for (sp::Coord &coord : routed_cells) {
    // remove the specific entry in the connectivity map
    int removed = grid->connMap()->remove(coord, conn); 
    if (!removed) {
      qFatal("A coord -- connection key value pair is not found in the grid at removal.");
    }
    if (!grid->connMap()->contains(coord) && grid->cellAt(coord)->getType() != sp::PinCell) {
      // if the cell doesn't have other connections running through, set to blank
      grid->cellAt(coord)->setType(sp::BlankCell);
      grid->cellAt(coord)->setPinSetId(-1);
      if (record_keeper != nullptr) {
        record_keeper->logCellGrid(grid, LogAllIntermediate, VisualizeAllIntermediate);
      }
    }
  }
}
