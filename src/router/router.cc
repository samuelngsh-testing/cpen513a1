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

bool Router::routeSuiteRipReroute(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid,
    bool *soft_halt, SolveCollection *solve_col)
{
  // prepare record keeping
  records->setSolveCollection(solve_col);
  records->newSolveSteps();
  
  // prepare variables before routing
  RoutingAlg *alg;                // algorithm to use
  QSet<sp::Coord> unrouted_pins;  // keep track of which pins have yet to be routed
  QMultiMap<int, sp::PinPair> map_pin_sets; // effectively sort pairs of pins by distance
  routePrep(pin_sets, map_pin_sets, unrouted_pins, &alg);

  // make copies of variables that need to be reset after full routing attempts
  sp::Grid *cell_grid_cp = new sp::Grid(cell_grid);
  QMultiMap<int, sp::PinPair> map_pin_sets_cp = map_pin_sets;

  // runtime settings and flags
  bool all_done = false;
  int attempts_left = (settings.net_reordering) ? settings.max_rerun_count : 1;
  QQueue<sp::PinPair> priority_routes;
  QSet<sp::Coord> failed_pins;
  QList<sp::PinPair> difficult_pairs;
  QMap<sp::PinPair, int> difficult_pair_failure_count;

  // high level routing loop
  while (!(*soft_halt) && !all_done && attempts_left > 0 && !map_pin_sets.isEmpty()) {
    // decide the source & sink to route
    sp::Coord source_coord, sink_coord;
    sp::PinPair pin_pair;
    if (!priority_routes.isEmpty()) {
      // take from the priority queue first
      pin_pair = priority_routes.dequeue();
    } else {
      // take from mapped pins
      pin_pair = map_pin_sets.take(map_pin_sets.firstKey());
    }
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

    // try to find a route
    bool success = routePinPair(alg, pin_pair, cell_grid);
    /* TODO remove
    bool success;
    QList<sp::Coord> route;
    if (cell_grid->routeExistsBetweenPins(source_coord, sink_coord, &route)) {
      success = true;
      createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(),
          cell_grid, records);
    } else {
      success = false;
      QList<sp::Connection*> rip_cand;
      QList<sp::Connection*> rip_blacklist;
      RouteResult result;
      result = alg->findRoute(source_coord, sink_coord, cell_grid, 
          settings.routed_cells_lower_cost, false, settings.rip_and_reroute,
          &rip_blacklist, records);
      route = result.route_coords;
      if (!route.isEmpty()) {
        if (!result.requires_rip) {
          // straightforward result that doesn't require ripping
          success = true;
          createConnection(pin_pair, route, (*cell_grid)(source_coord)->pinSetId(), 
              cell_grid, records);
          cell_grid->clearWorkingValues();
          records->logCellGrid(cell_grid, LogResultsOnly, VisualizeResultsOnly);
        } else if (settings.rip_and_reroute) {
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
            records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
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
              records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
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
            records->logCellGrid(cell_grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
          } else {
            success = true;
          }
        }
      } else {
        cell_grid->clearWorkingValues();
        records->logCellGrid(cell_grid, LogResultsOnly, VisualizeResultsOnly);
      }
    }
    */

    // route failure remedies
    if (!success) {
      if (!failed_pins.contains(source_coord) && !failed_pins.contains(sink_coord)) {
        difficult_pair_failure_count[pin_pair] += 1;
        if (!difficult_pairs.contains(pin_pair)) {
          // every time a new failure emerges, put them as top priority for the next round
          difficult_pairs.prepend(pin_pair);
        } else if (difficult_pair_failure_count[pin_pair] == settings.difficult_boost_thresh) {
          // boost order of difficult pairs after a certain failure threshold
          difficult_pairs.removeOne(pin_pair);
          difficult_pairs.prepend(pin_pair);
          difficult_pair_failure_count[pin_pair] = 0;
        }
        failed_pins.insert(source_coord);
        failed_pins.insert(sink_coord);
      }
    }

    // end of main loop
    if (map_pin_sets.isEmpty() && failed_pins.isEmpty()) {
      // all pins have been routed without failure, all done
      all_done = true;
    } else if (map_pin_sets.isEmpty() && !failed_pins.isEmpty()) {
      // there were failures, handle high level logic updates
      for (auto difficult_pair : difficult_pairs) {
        priority_routes.enqueue(difficult_pair);
      }
      cell_grid->copyState(cell_grid_cp);
      map_pin_sets = map_pin_sets_cp;
      failed_pins.clear();
      attempts_left--;
      qDebug() << tr("****No solution found, attempts left: %1****").arg(attempts_left);
      if (attempts_left > 0) {
        records->newSolveSteps();
      }
    }
  }

  // if main loop thinks it's done, perform sanity checks
  if (all_done) {
    bool cell_grid_routed = cell_grid->allPinsRouted();
    if (all_done != cell_grid_routed) {
      qWarning() << tr("Route success state differs between program and Grid's "
          "return. Program thinks %1, Grid thinks %2.").arg(all_done)
        .arg(cell_grid_routed);
    } else {
      qDebug() << tr("ALL ROUTES COMPLETED SUCCESSFULLY.");
    }
  }

  delete cell_grid_cp;
  delete alg;

  return all_done;
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

void Router::routePrep(const QList<sp::PinSet> &pin_sets, 
    QMultiMap<int,sp::PinPair> &map_pin_sets, QSet<sp::Coord> &unrouted_pins,
    RoutingAlg **alg)
{
  // choose the algorithm to use
  switch (settings.use_alg) {
    case LeeMoore:
      (*alg) = new LeeMooreAlg();
      break;
    case AStar:
    default:
      (*alg) = new AStarAlg();
      break;
  }

  // initialize a map that sorts pin sets from nearest to farthest as well as
  // a set that stores unrouted pins
  for (const sp::PinSet &pin_set : pin_sets) {
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
}

bool Router::routePinPair(RoutingAlg *alg, const sp::PinPair &pin_pair,
    sp::Grid *grid)
{
  bool success = false;
  sp::Coord source_coord = pin_pair.first;
  sp::Coord sink_coord = pin_pair.second;
  QSet<sp::Coord> all_routed_coords;
  QList<sp::Connection*> rip_cand;
  QList<sp::Connection*> rip_blacklist;

  // end early if a connection already exists
  QList<sp::Coord> route;
  if (grid->routeExistsBetweenPins(source_coord, sink_coord, &route)) {
    createConnection(pin_pair, route, (*grid)(source_coord)->pinSetId(),
        grid, records);
    return true;
  }

  // attempt to route from source coord to sink
  RouteResult result = alg->findRoute(source_coord, sink_coord, grid,
      settings.routed_cells_lower_cost, false, settings.rip_and_reroute,
      &rip_blacklist, records);
  all_routed_coords.fromList(result.route_coords);

  if (!result.route_coords.isEmpty() && !result.requires_rip) {
    // straightforward result that doesn't require ripping
    success = true;
    createConnection(pin_pair, result.route_coords, (*grid)(source_coord)->pinSetId(),
        grid, records);
    grid->clearWorkingValues();
    records->logCellGrid(grid, LogResultsOnly, VisualizeResultsOnly);
  } else if (settings.rip_and_reroute) {
    // attempt rip and reroute
    int rip_attempts_left = settings.rip_and_rerout_count;

    // save the grid before doing anything
    grid->clearWorkingValues();
    sp::Grid grid_pre_rip(grid);

    while (rip_attempts_left > 0 && !result.route_coords.isEmpty()) {
      // get the connections that need to be ripped to make the route
      QList<sp::PinPair> pairs_to_reroute;
      QSet<sp::Connection*> conns = existingConnections(result.route_coords,
          grid, (*grid)(source_coord)->pinSetId());
      qDebug() << tr("Attempting rip and reroute with %1 routes to rip.").arg(conns.count());
      // rip and keep a record of what was ripped
      for (sp::Connection *conn : conns) {
        pairs_to_reroute.append(conn->pinPair());
        ripConnection(conn, grid, records);
        delete conn;
        records->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
      }

      // create the new connection
      createConnection(pin_pair, result.route_coords,
          (*grid)(source_coord)->pinSetId(), grid, records);

      // reroute the ripped connections
      bool all_rerouted = true;
      for (const sp::PinPair &reroute_pair : pairs_to_reroute) {
        RouteResult reroute_result;
        reroute_result = alg->findRoute(reroute_pair.first, reroute_pair.second,
            grid, settings.routed_cells_lower_cost, false, false,
            &rip_blacklist, records);
        if (!reroute_result.route_coords.isEmpty()) {
          createConnection(reroute_pair, reroute_result.route_coords, 
              (*grid)(reroute_pair.first)->pinSetId(), grid, records);
          grid->clearWorkingValues();
          records->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
        } else {
          grid->clearWorkingValues();
          qDebug() << "Rip and reroute failed because one of the ripped routes "
            "cannot be rerouted.";
          all_rerouted = false;
          break;
        }
      }

      // revert if not all ripped connections can be rerouted
      // add ripped connections to blacklist, if there is quota for reattempt
      // then these routes won't be reused
      if (!all_rerouted) {
        qDebug() << "Reverting to state prior to rerouting";
        grid->copyState(&grid_pre_rip);
        records->logCellGrid(grid, LogCoarseIntermediate, VisualizeCoarseIntermediate);
        rip_blacklist = QList<sp::Connection*>::fromSet(existingConnections(
              all_routed_coords.toList(), grid, (*grid)(source_coord)->pinSetId()));

        // rerun the routing with the new blacklist
        result = alg->findRoute(source_coord, sink_coord, grid,
            settings.routed_cells_lower_cost, false, settings.rip_and_reroute,
            &rip_blacklist, records);
      } else {
        success = true;
      }

      rip_attempts_left--;
    }
  } else {
    grid->clearWorkingValues();
    records->logCellGrid(grid, LogResultsOnly, VisualizeResultsOnly);
  }

  return success;
}
