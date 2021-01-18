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
Router::Router(const Problem &problem, const QString &cache_path, 
    RouterSettings settings)
  : problem(problem), settings(settings)
{
  // attempt to create the cache path
  QDir cache_dir(cache_path);
  qDebug() << tr("Attempting to create cache path %1...").arg(cache_path);
  if (!cache_dir.mkpath(".")) {
    qFatal("Unable to create cache path.");
  }
}

bool Router::routeSuite(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid, 
    SolveCollection *solve_col)
{
  bool all_succeeded=false;

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
      qDebug() << tr("Has existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      success = true;
    } else {
      qDebug() << tr("No existing route between %1 and %2").arg(source_coord.str()).arg(sink_coord.str());
      success = routeForId(AStar, {pin_pair.first, pin_pair.second}, cell_grid, solve_col);
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

  return all_succeeded;
}

bool Router::routeForId(RouteAlg alg, sp::PinSet pin_set, sp::Grid *cell_grid,
    SolveCollection *solve_col)
{
  bool successful = false;

  if (alg==LeeMoore) {
    // TODO current implementation naively routes pin sets in the order that was
    // provided. Some intelligence should be exercised in choosing the order.

    for (int pin_id=0; pin_id<pin_set.size()-1; pin_id++) {
      SolveSteps *solve_steps = nullptr;
      if (solve_col != nullptr) {
        solve_steps = solve_col->newSolveSteps();
      }
      sp::Coord source_coord = pin_set[pin_id+1];
      sp::Coord sink_coord = pin_set[pin_id];
      successful = leeMoore(source_coord, sink_coord, cell_grid, solve_steps);
    }
  } else if (alg==AStar) {
    for (int pin_id=0; pin_id<pin_set.size()-1; pin_id++) {
      SolveSteps *solve_steps = nullptr;
      if (solve_col != nullptr) {
        solve_steps = solve_col->newSolveSteps();
      }
      // TODO add generic pins
      sp::Coord source_coord = pin_set[pin_id+1];
      sp::Coord sink_coord = pin_set[pin_id];
      qDebug() << tr("Routing from %1 to %2").arg(source_coord.str()).arg(sink_coord.str());
      successful = aStar(source_coord, sink_coord, cell_grid, solve_steps);
    }
  }

  return successful;
}

bool Router::leeMoore(const sp::Coord &source_coord, const sp::Coord &sink_coord,
    sp::Grid *cell_grid, SolveSteps *solve_steps, int pin_set_id)
{
  // lambda function that marks all neighboring cells of a given coordinate
  // returns a list of neighbors that were successfully marked
  auto markNeighbors = [](const sp::Coord &coord, sp::Grid *cell_grid, 
      int pin_set_id, bool &marked)->QList<sp::Coord>
  {
    marked=false;
    QList<sp::Coord> neighbors = cell_grid->neighborCoordsOf(coord);
    // mark each neighbor if eligible
    for (const sp::Coord &neighbor : neighbors) {
      sp::Cell *cell = cell_grid->cellAt(neighbor);
      if (cell->workingValue() < 0 
          && (cell->getType() == sp::BlankCell
            || cell->pinSetId() == pin_set_id)) {
        // eligible neighbor found
        cell->setWorkingValue(cell_grid->cellAt(coord)->workingValue()+1);
        marked=true;
      } else {
        // discard ineligible neighbor
        neighbors.removeOne(neighbor);
      }
    }
    return neighbors;
  };

  // lambda function that recursively marks neighboring cells contageously from
  // the source until the specified sink is reached or until no more neighbors
  // are available for marking.
  auto runLeeMoore = [this, &markNeighbors, &source_coord, &sink_coord, 
       &cell_grid, solve_steps](int pin_set_id)->bool
  {
    bool marked;
    // mark neighbors of the source
    QList<sp::Coord> neighbors = markNeighbors(source_coord, cell_grid, pin_set_id, marked);
    logCellGrid(cell_grid, solve_steps, LogAllIntermediate);
    // loop through neighbors list until sink found
    while (!neighbors.isEmpty()) {
      sp::Coord base_coord = neighbors.takeFirst();
      if (base_coord == sink_coord) {
        // return if a connection has been made to the sink
        qDebug() << "Route to sink found.";
        return true;
      }
      // keep marking more neighbors
      neighbors.append(markNeighbors(base_coord, cell_grid, pin_set_id, marked));
      if (marked) {
        logCellGrid(cell_grid, solve_steps, LogAllIntermediate);
      }
    }
    // reaching this point means that no solution was found
    return false;
  };

  // lambda function that runs the back-propagation recursively after cells 
  // have been marked appropriately
  std::function<void(const sp::Coord &, sp::Grid *, int)> runBackprop;
  runBackprop = [this, &source_coord, &runBackprop, solve_steps]
    (const sp::Coord &curr_coord, sp::Grid *cell_grid, int pin_set_id)
  {
    if (curr_coord == source_coord) {
      qDebug() << "Back propagation complete.";
      return;
    } else {
      int curr_working_val = cell_grid->cellAt(curr_coord)->workingValue();
      for (sp::Cell *nc : cell_grid->neighborsOf(curr_coord)) {
        if (nc->workingValue() == 0) {
          qDebug() << "Back propagation complete.";
          return;
        } else if (nc->workingValue() == curr_working_val - 1) {
          // update cell type for chosen routed cells
          nc->setType(sp::RoutedCell);
          nc->setPinSetId(pin_set_id);
          logCellGrid(cell_grid, solve_steps, LogAllIntermediate);
          // recurse until source reached
          runBackprop(nc->getCoord(), cell_grid, pin_set_id);
          break;
        }
      }
    }
  };

  // check that the provided source and sink locations are both pins and have 
  // the same pin_set_id
  sp::Cell *source_cell = cell_grid->cellAt(source_coord);
  sp::Cell *sink_cell = cell_grid->cellAt(sink_coord);
  sp::CellType source_type = source_cell->getType();
  sp::CellType sink_type = sink_cell->getType();
  if (pin_set_id < 0) {
    // pin set ID was not specified, assume routing between pins
    // take ID from source/sink
    pin_set_id = cell_grid->cellAt(source_coord)->pinSetId();
    //assert(source_type == sp::PinCell);
    //assert(sink_type == sp::PinCell);
    assert(pin_set_id == cell_grid->cellAt(sink_coord)->pinSetId());
  } else {
    // pin set ID was specified, check that the ID is identical to those of the
    // source/sink if they're pins or routed cells
    if (source_type == sp::PinCell || source_type == sp::RoutedCell) {
      assert(source_cell->pinSetId() == pin_set_id);
    }
    if (sink_type == sp::PinCell || sink_type == sp::RoutedCell) {
      assert(sink_cell->pinSetId() == pin_set_id);
    }
  }

  // set the working value at the source pin to be 0
  source_cell->setWorkingValue(0);

  // run Lee Moore forward pass
  qDebug() << tr("Running Lee-Moore from %1 to %2").arg(source_coord.str())
    .arg(sink_coord.str());
  bool success = runLeeMoore(pin_set_id);

  // run back propagation
  if (success) {
    runBackprop(sink_coord, cell_grid, pin_set_id);
  } else {
    qDebug() << tr("Failed to run Lee-Moore on pin set %1").arg(pin_set_id);
  }
  logCellGrid(cell_grid, solve_steps, LogCoarseIntermediate);

  // clear all working values
  cell_grid->clearWorkingValues();
  logCellGrid(cell_grid, solve_steps, LogResultsOnly);

  return success;
}

bool Router::aStar(const sp::Coord &source_coord, const sp::Coord &sink_coord,
    sp::Grid *cell_grid, SolveSteps *solve_steps)
{
  // lambda function that marks all neighboring cells of a given coordinate by A*
  // returns a list of neighbors that were successfully marked
  // if a valid termination is found (whether because it's the sink or because 
  // it's a routed cell with the same pin id), then termination is set
  auto markNeighbors = [&source_coord, &sink_coord](const sp::Coord &coord, 
      sp::Grid *cell_grid, int pin_set_id, bool &marked, sp::Coord &termination)
    ->QMultiMap<QPair<int,int>,sp::Coord>
  {
    marked = false;
    termination = sp::Coord();
    QMultiMap<QPair<int,int>,sp::Coord> expl_map;
    QList<sp::Coord> neighbors = cell_grid->neighborCoordsOf(coord);
    // mark each neighbor if eligible
    for (const sp::Coord &neighbor : neighbors) {
      sp::Cell *nc = cell_grid->cellAt(neighbor);
      if (nc->getType() == sp::BlankCell || nc->pinSetId() == pin_set_id) {
        // eligible neighbor found
        int d_from_source = cell_grid->cellAt(coord)->extraProps()["d_from_source"].toInt();
        if (nc->getType() == sp::RoutedCell && nc->pinSetId() == pin_set_id) {
          d_from_source += 40;
        } else {
          d_from_source += 100;
        }
        int md_sink = 100*neighbor.manhattanDistance(sink_coord);
        int priority = coord.manhattanDistance(sink_coord);
        int working_val = d_from_source + md_sink;
        if (nc->workingValue() < 0 || nc->workingValue() > working_val) {
          nc->setWorkingValue(working_val);
          // update values in the newly traversed neighbor
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
          // bookkeeping
          marked = true;
          sp::Cell *nc = cell_grid->cellAt(neighbor);
          if (neighbor == sink_coord 
              || (nc->getType() == sp::RoutedCell
                && cell_grid->routeExistsBetweenPins(neighbor, sink_coord))) {
          //if (neighbor == sink_coord) {
            termination = neighbor;
          }
        }
      }
    }
    return expl_map;
  };

  // lambda function that runs the A* algorithm until destination has been 
  // reached
  auto runAStar = [this, &markNeighbors, &source_coord, &sink_coord, cell_grid,
       solve_steps](int pin_set_id, sp::Coord &termination)->bool
  {
    bool marked;
    // keep a map of neighbors to be explored with the key being the distance
    // from source (automatically sorted)
    QMultiMap<QPair<int,int>,sp::Coord> expl_map;
    // add the source coord as the first element to look at
    int md = source_coord.manhattanDistance(sink_coord);
    expl_map.insert(qMakePair(md,0), source_coord);
    cell_grid->cellAt(source_coord)->extraProps()["d_from_source"] = 0;
    cell_grid->cellAt(source_coord)->setWorkingValue(md*100);
    // loop through neighbors list until sink found
    while (!expl_map.isEmpty()) {
      // take the coordinate with the minimum working value
      sp::Coord coord_mwv = expl_map.take(expl_map.firstKey());
      // insert newly found neighbors to the exploration map
      expl_map.unite(markNeighbors(coord_mwv, cell_grid, pin_set_id, marked, termination));
      if (marked) {
        logCellGrid(cell_grid, solve_steps, LogAllIntermediate);
      }
      if (!termination.isBlank()) {
        return true;
      }
    }
    // reaching this point means that no solution was found
    return false;
  };

  // lambda function that performs backtracing to create the route
  std::function<void(const sp::Coord &, sp::Grid *, int)> backtrace;
  backtrace = [this, &source_coord, &backtrace, solve_steps]
    (const sp::Coord &curr_coord, sp::Grid *cell_grid, int pin_set_id)
  {
    if (curr_coord == source_coord) {
      qDebug() << "Back tracing complete.";
      return;
    } else {
      QVariant from_coord_v = cell_grid->cellAt(curr_coord)->extraProps()["from_coord"];
      sp::Coord from_coord = from_coord_v.value<sp::Coord>();
      sp::Cell *from_cell = cell_grid->cellAt(from_coord);
      if (from_cell->getType() != sp::PinCell) {
        from_cell->setType(sp::RoutedCell);
        from_cell->setPinSetId(pin_set_id);
      }
      if (cell_grid->routeExistsBetweenPins(from_coord, source_coord)) {
        qDebug() << "Back tracing ended early because connection already made.";
        return;
      }
      logCellGrid(cell_grid, solve_steps, LogAllIntermediate);
      backtrace(from_coord, cell_grid, pin_set_id);
    }
  };

  sp::Coord termination;  // might find valid termination that's not sink
  int pin_set_id = cell_grid->cellAt(source_coord)->pinSetId();
  bool success = runAStar(pin_set_id, termination);

  if (success) {
    backtrace(termination, cell_grid, pin_set_id);
  }
  logCellGrid(cell_grid, solve_steps, LogCoarseIntermediate);

  // clear all working values
  cell_grid->clearWorkingValues();
  logCellGrid(cell_grid, solve_steps, LogResultsOnly);

  return success;
}

void Router::logCellGrid(sp::Grid *cell_grid, SolveSteps *solve_steps, 
    LogVerbosity detail)
{
  if (detail < settings.detail_level) {
    return;
  }
  // log the provided cell grid if both provided pointers are not nullptrs.
  if (cell_grid != nullptr && solve_steps != nullptr) {
    solve_steps->step_grids.append(new sp::Grid(cell_grid));
  }
};
