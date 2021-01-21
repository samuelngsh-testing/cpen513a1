// @file:     router_records.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Implementation of classes relevant to storing routing records.

#include "routing_records.h"

using namespace rt;

SolveSteps *RoutingRecords::newSolveSteps()
{
  curr_solve_steps = solve_col->newSolveSteps();
  return curr_solve_steps;
}

void RoutingRecords::logCellGrid(sp::Grid *cell_grid, LogVerbosity detail,
    GuiUpdateVerbosity gui_detail)
{
  if (gui_detail >= gui_verbosity) {
    emit routerStep(cell_grid);
  }
  if (detail < log_verbosity) {
    return;
  }
  // log the provided cell grid if both provided pointers are not nullptrs.
  if (cell_grid != nullptr && curr_solve_steps != nullptr) {
    curr_solve_steps->step_grids.append(new sp::Grid(cell_grid));
  }
};
