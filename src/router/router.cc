// @file:     router.cc
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Router class.

#include <QFile>
#include <QDir>
#include <QDebug>
#include "router.h"

using namespace rt;

// Constructor
Router::Router(const Problem &problem, const QString &cache_path)
  : problem(problem)
{
  // attempt to create the cache path
  QDir cache_dir(cache_path);
  qDebug() << tr("Attempting to create cache path %1...").arg(cache_path);
  if (!cache_dir.mkpath(".")) {
    qFatal("Unable to create cache path.");
  }
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
      sp::Coord source_coord = pin_set[pin_id];
      sp::Coord sink_coord = pin_set[pin_id+1];
      successful = leeMoore(source_coord, sink_coord, cell_grid, solve_steps);
    }
  }

  return successful;
}

bool Router::leeMoore(const sp::Coord &source_coord, const sp::Coord &sink_coord,
    sp::Grid *cell_grid, SolveSteps *solve_steps)
{
  // lambda function that clones the provided cell_grid to the solve_steps record
  // for later step-by-step review.
  auto logCellGrid = [solve_steps](sp::Grid *cell_grid)
  {
    if (solve_steps != nullptr) {
      solve_steps->step_grids.append(new sp::Grid(cell_grid));
    }
  };

  // lambda function that marks all neighboring cells for a given coordinate
  // returns a list of neighbors that were successfully marked
  auto markNeighbors = [](sp::Coord coord, sp::Grid *cell_grid, 
      int pin_set_id, bool &marked)->QList<sp::Coord>
  {
    marked=false;
    QList<sp::Coord> neighbors = cell_grid->neighborCoordsOf(coord);
    for (const sp::Coord &neighbor : neighbors) {
      sp::Cell *cell = cell_grid->cellAt(neighbor);
      if (cell->workingValue() < 0 
          && (cell->getType() == sp::BlankCell
            || cell->pinSetId() == pin_set_id)) {
        cell->setWorkingValue(cell_grid->cellAt(coord)->workingValue()+1);
        marked=true;
      } else {
        neighbors.removeOne(neighbor);
      }
    }
    return neighbors;
  };

  // lambda function that recursively marks neighboring cells contageously from
  // the source until the specified sink is reached or until no more neighbors
  // are available for marking.
  auto runLeeMoore = [&markNeighbors, &source_coord, &sink_coord, &cell_grid,
       &logCellGrid]
    (int pin_set_id)->bool
  {
    bool marked;
    QList<sp::Coord> neighbors = markNeighbors(source_coord, cell_grid, pin_set_id, marked);
    logCellGrid(cell_grid);
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
        logCellGrid(cell_grid);
      }
    }
    // reaching this point means that no solution was found
    return false;
  };

  // lambda function that runs the back-propagation after cells have been
  // marked appropriately
  std::function<void(const sp::Coord &, sp::Grid *, int)> runBackprop;
  runBackprop = [&source_coord, &runBackprop, &logCellGrid]
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
          logCellGrid(cell_grid);
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
  int pin_set_id = cell_grid->cellAt(source_coord)->pinSetId();
  assert(source_cell->getType() == sp::PinCell);
  assert(sink_cell->getType() == sp::PinCell);
  assert(pin_set_id == cell_grid->cellAt(sink_coord)->pinSetId());

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

  // clear all working values
  cell_grid->clearWorkingValues();
  logCellGrid(cell_grid);

  return success;
}
