// @file:     spatial.cc
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Implementation of spatial classes.

#include <QDebug>
#include "spatial.h"

using namespace sp;

// Coord class implementations

bool Coord::operator==(const Coord &other) const
{
  return (x == other.x && y == other.y);
}

int Coord::manhattanDistance(const Coord &other) const
{
  if (isBlank() || other.isBlank())
    qFatal("Attempted to find Manhattan distance between one or more invalid coordinates");
  return abs(other.x - x) + abs(other.y - y);
}

bool Coord::isWithinBounds(int x_max, int y_max) const
{
  return (!is_blank) && (x >= 0 && x < x_max && y >=0 && y < y_max);
}

Grid::Grid(int dim_x, int dim_y, const QList<Coord> &obs_coords, 
    const QList<PinSet> &pin_sets)
  : dim_x(dim_x), dim_y(dim_y)
{
  // initiate the cell grid
  cell_grid.resize(dim_x);
  for (int i=0; i<dim_x; i++) {
    cell_grid[i].resize(dim_y);
    for (int j=0; j<dim_y; j++) {
      // init grid cell to blank cell
      cell_grid[i][j] = new Cell(Coord(i,j), BlankCell);
    }
  }

  // set cell properties
  setObsCells(obs_coords);
  for (int id=0; id<pin_sets.size(); id++) {
    setPinCells(pin_sets[id], id);
  }
}

Grid::Grid(Grid *other)
{
  int dim_x = other->cell_grid.size();
  int dim_y = other->cell_grid[0].size();
  cell_grid.resize(dim_x);
  for (int i=0; i<dim_x; i++) {
    cell_grid[i].resize(dim_y);
    for (int j=0; j<dim_y; j++) {
      cell_grid[i][j] = new Cell(*(other->cellAt(Coord(i,j))));
    }
  }
}

Grid::~Grid()
{
  clearGrid();
}

void Grid::copyState(Grid *other)
{
  dim_x = other->dim_x;
  dim_y = other->dim_y;
  pin_sets = other->pin_sets;
  cell_grid.clear();
  cell_grid.resize(other->cell_grid.size());
  for (int i=0; i<cell_grid.size(); i++) {
    cell_grid[i].resize(other->cell_grid[i].size());
    for (int j=0; j<cell_grid[i].size(); j++) {
      cell_grid[i][j] = new Cell(*other->cell_grid[i][j]);
    }
  }
  conn.clear();
  QMap<sp::Connection*,sp::Connection*> old_to_new_ptr;
  for (auto it=other->conn.begin(); it!=other->conn.end(); it++) {
    auto find_ptr = old_to_new_ptr.constFind(it.value());
    if (find_ptr != old_to_new_ptr.constEnd()) {
      // re-insert the pointer if it had been created
      conn.insert(it.key(), find_ptr.value());
    } else {
      // create the pointer and keep track of it
      sp::Connection *nconn = new sp::Connection(it.value());
      old_to_new_ptr[it.value()] = nconn;
      conn.insert(it.key(), nconn);
    }
  }
}

void Grid::setObsCells(const QList<Coord> &obs_coords, bool check_clash)
{
  for (Coord coord : obs_coords) {
    Cell *cell = cellAt(coord);
    if (check_clash && cell->getType() != BlankCell) {
      qWarning() << QObject::tr("Potential cell clash detected at (%1, %2)")
        .arg(coord.x).arg(coord.y);
    }
    cellAt(coord)->setType(ObsCell);
  }
}

void Grid::setPinCells(const QList<Coord> &pin_coords, int pin_set_id, 
    bool check_clash)
{
  if (pin_sets.contains(pin_set_id)) {
    qFatal("This Grid already contains a set of pins with the specified ID.");
  }
  pin_sets[pin_set_id] = pin_coords;
  for (Coord coord : pin_coords) {
    Cell *cell = cellAt(coord);
    if (check_clash && cell->getType() != BlankCell) {
      qWarning() << QObject::tr("Potential cell clash detected at (%1, %2)")
        .arg(coord.x).arg(coord.y);
    }
    cell->setType(PinCell);
    cell->setPinSetId(pin_set_id);
  }
}

QList<Cell*> Grid::neighborsOf(const Coord &coord)
{
  QList<Cell*> neighbors;
  for (const Coord &coord : neighborCoordsOf(coord)) {
    neighbors.append(cellAt(coord));
  }
  return neighbors;
}

QList<Coord> Grid::neighborCoordsOf(const Coord &coord)
{
  QList<Coord> neighbors;
  for (const Coord &coord : {coord.above(), coord.right(), coord.below(), coord.left()}) {
    if (isWithinBounds(coord)) {
      neighbors.append(coord);
    }
  }
  return neighbors;
}

void Grid::clearWorkingValues()
{
  for (auto x_cells : cell_grid) {
    for (Cell *cell : x_cells) {
      cell->resetWorkingValue();
      cell->extraProps().clear();
    }
  }
}

bool Grid::isWithinBounds(const Coord &coord)
{
  return (coord.x >= 0 && coord.y >= 0 && coord.x < dim_x && coord.y < dim_y);
}

bool Grid::routeExistsBetweenPins(const Coord &a, const Coord &b, 
    QList<sp::Coord> *route)
{
  int pin_set_id = cellAt(a)->pinSetId();
  assert(cellAt(b)->pinSetId() == pin_set_id);

  // operate on a copy of the grid
  Grid grid_cp(this);
  grid_cp.clearWorkingValues();
  grid_cp(a)->setWorkingValue(1); // prevent coord a from being revisited

  // recursively look at neighboring RoutedCells/Pins starting from point a
  // until point b is found.
  std::function<bool(const Coord &curr_coord)> findB;
  findB = [this, &a, &b, &grid_cp, &pin_set_id, route, &findB](const Coord &curr_coord)->bool
  {
    if (curr_coord == b) {
      return true;
    } else {
      QList<Coord> neighbors = neighborCoordsOf(curr_coord);
      for (Coord neighbor : neighbors) {
        Cell *nc = grid_cp(neighbor);
        CellType type = nc->getType();
        if (nc->workingValue() < 0 && nc->pinSetId() == pin_set_id
            && (type == RoutedCell || type == PinCell)) {
          nc->setWorkingValue(1);
          bool success = findB(neighbor);
          if (success) {
            if (route != nullptr && a != curr_coord) {
              route->append(curr_coord);
            }
            return true;
          }
        }
      }
    }
    return false;
  };

  return findB(a);
}

bool Grid::allPinsRouted()
{
  for (PinSet pin_set : pin_sets) {
    for (int i=0; i<pin_set.size()-1; i++) {
      if (!routeExistsBetweenPins(pin_set[i], pin_set[i+1])) {
        return false;
      }
    }
  }
  return true;
}

void Grid::clearGrid()
{
  // destroy all cells in the grid
  for (auto x_cells : cell_grid) {
    x_cells.clear();
  }
  cell_grid.clear();

  dim_x = 0;
  dim_y = 0;
  pin_sets.clear();
}
