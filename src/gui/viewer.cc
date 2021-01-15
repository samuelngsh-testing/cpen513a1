// @file:     viewer.cc
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the viewer class.

#include "viewer.h"

using namespace gui;
using namespace settings;

// constructor
Viewer::Viewer(QWidget *parent)
  : QGraphicsView(parent)
{
  initViewer();
}

Viewer::~Viewer()
{
  clearProblem();
}

// show a provided problem
void Viewer::showProblem(const rt::Problem &problem)
{
  clearProblem();
  curr_problem = problem;

  // create a graphical cell at every part of the grid
  sp::Coord dims = curr_problem.dimensions();
  for (int x=0; x<dims.x; x++) {
    for (int y=0; y<dims.y; y++) {
      Cell *cell = new Cell(curr_problem.cellGrid()->cellAt(sp::Coord(x,y)));
      cells.append(cell);
      scene->addItem(cell);
    }
  }

  fitProblemInView();
}

// clear problem from viewer
void Viewer::clearProblem()
{
  for (Cell *cell : cells) {
    scene->removeItem(cell);
    delete cell;
  }
  cells.clear();
  curr_problem = rt::Problem();
}

// initialize viewer GUI
void Viewer::initViewer()
{
  scene = new QGraphicsScene(this);
  setScene(scene);
}

void Viewer::fitProblemInView()
{
  if (curr_problem.isValid()) {
    sp::Coord dims = curr_problem.dimensions();
    fitInView(QRectF(0, 0, dims.x*Settings::sf, dims.y*Settings::sf), Qt::KeepAspectRatio);
  }
}

void Viewer::updateCellGrid(sp::Grid *cell_grid)
{
  if (cell_grid == nullptr) {
    cell_grid = curr_problem.cellGrid();
  }
  for (Cell *cell : cells) {
    cell->updateCell(cell_grid->cellAt(cell->getCoord()));
  }
  scene->update();
}
