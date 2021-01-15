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
  if (!isBlank() || !other.isBlank())
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
  // destroy all cells in the grid
  for (auto x_cells : cell_grid) {
    x_cells.clear();
  }
  cell_grid.clear();
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
    }
  }
}

bool Grid::isWithinBounds(const Coord &coord)
{
  return (coord.x >= 0 && coord.y >= 0 && coord.x < dim_x && coord.y < dim_y);
}
