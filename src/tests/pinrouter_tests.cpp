// @file:     problem_tests.cpp
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Unit tests that ensure the Problem class reads a known problem 
//            file properly into the needed lists and maps.

#include <QtTest/QtTest>
#include "router/problem.h"
#include "router/router.h"
#include "gui/settings.h"

class RouterTests : public QObject
{
  Q_OBJECT

  public:

    //! Function for checking that cells specified in the provided list are all
    //! of the specified cell type. Also checks that Pins and RoutedCells
    //! belong to the specified pin_set_id.
    bool checkTypeOfCells(const QList<sp::Coord> &coords, sp::Grid *grid,
        sp::CellType type, int pin_set_id=-1)
    {
      for (const sp::Coord &coord : coords) {
        sp::Cell *cell = grid->cellAt(coord);
        sp::CellType t = cell->getType();
        int psid = cell->pinSetId();
        if (t != type) {
          return false;
        } else if ((t == sp::PinCell || t == sp::RoutedCell) && psid != pin_set_id) {
          return false;
        }
      }
      return true;
    }

  // functions in these slots are automatically called after compilation
  private slots:

    //! Test the capability to read problems and create appropriate data 
    //! structures.
    void testProblemRead()
    {
      using namespace rt;

      // a minimal problem for testing
      // Looks like:
      // P          
      // P         P
      //           P
      Problem problem(":/test_problems/3_rows.infile");
      QCOMPARE(problem.isValid(), true);
      // x and y dimensions
      QCOMPARE(problem.dimensions().x, 11);
      QCOMPARE(problem.dimensions().y, 3);
      QList<sp::Coord> obs;   // no obstruction cells for this problem
      QList<QList<sp::Coord>> pins({{sp::Coord(0,0), sp::Coord(10,1)},
          {sp::Coord(0,1), sp::Coord(10,2)}});
      QList<sp::Coord> blanks;
      for (int x=0; x<11; x++) {
        for (int y=0; y<3; y++) {
          sp::Coord coord(x,y);
          if (!pins[0].contains(coord) && !pins[1].contains(coord) 
              && !obs.contains(coord)) {
            blanks.append(sp::Coord(x,y));
          }
        }
      }
      sp::Grid *grid = problem.cellGrid();
      QCOMPARE(checkTypeOfCells(obs, grid, sp::ObsCell), true);
      QCOMPARE(checkTypeOfCells(pins[0], grid, sp::PinCell, 0), true);
      QCOMPARE(checkTypeOfCells(pins[1], grid, sp::PinCell, 1), true);
      QCOMPARE(checkTypeOfCells(blanks, grid, sp::BlankCell), true);
      grid = nullptr;

      // similar problem as above but with obstruction cells
      // Looks like:
      // P   OOO   P
      // (3 obstruction cells in the middle)
      problem = Problem(":/test_problems/3_rows_w_obs.infile");
      QCOMPARE(problem.isValid(), true);
      // x and y dimensions
      QCOMPARE(problem.dimensions().x, 11);
      QCOMPARE(problem.dimensions().y, 3);
      obs = QList<sp::Coord>({sp::Coord(5,0), sp::Coord(5,1), sp::Coord(5,2)});
      pins = QList<QList<sp::Coord>>({{sp::Coord(0,0), sp::Coord(10,1)},
          {sp::Coord(0,1), sp::Coord(10,2)}});
      blanks.clear();
      for (int x=0; x<11; x++) {
        for (int y=0; y<3; y++) {
          sp::Coord coord(x,y);
          if (!pins[0].contains(coord) && !pins[1].contains(coord) 
              && !obs.contains(coord)) {
            blanks.append(sp::Coord(x,y));
          }
        }
      }
      grid = problem.cellGrid();
      QCOMPARE(checkTypeOfCells(obs, grid, sp::ObsCell), true);
      QCOMPARE(checkTypeOfCells(pins[0], grid, sp::PinCell, 0), true);
      QCOMPARE(checkTypeOfCells(pins[1], grid, sp::PinCell, 1), true);
      QCOMPARE(checkTypeOfCells(blanks, grid, sp::BlankCell), true);
    }


    //! Test the capability to route the problems above (or appropriately fail 
    //! for unroutable problems).
    void testProblemRoute()
    {
      using namespace rt;

      // 3 rows problem routing (without obstruction)
      Problem problem(":/test_problems/3_rows.infile");
      // mostly default settings but specify coarse grid logging
      RouterSettings settings;
      settings.log_level=LogCoarseIntermediate;
      // route the problem
      Router *router = new Router(problem, settings);
      bool soft_halt=false;
      SolveCollection solve_col;
      router->routeSuite(problem.pinSets(), problem.cellGrid(), &soft_halt,
          &solve_col);
      // validation
      sp::Grid *grid = problem.cellGrid();
      QCOMPARE(grid->allPinsRouted(), true);  // trivial problem must be successful
      int dim_x = 11;
      int dim_y = 3;
      for (int x=0; x<dim_x; x++) {
        int routed_cells = 0;
        for (int y=0; y<dim_y; y++) {
          if (grid->cellAt(sp::Coord(x, y))->getType() == sp::RoutedCell) {
            routed_cells++;
          }
        }

        // set the expected min and max routed cell counts for each column of
        // cells indexed by the x coordinate.
        int min_rcell_count;  // inclusive
        int max_rcell_count;  // inclusive
        if (x == 0 || x == 10) {
          // could have 0 RCell at the side edges
          min_rcell_count = 0;
          // at most, all empty space on the side edges are filled with RCells
          max_rcell_count = dim_y - problem.pinSets().size();
        } else {
          // at least one RCell per set of pins
          min_rcell_count = problem.pinSets().size();
          // at most the entire column might be filled
          max_rcell_count = dim_y;
        }
        QCOMPARE(routed_cells >= min_rcell_count, true);
        QCOMPARE(routed_cells <= max_rcell_count, true);
      }
      delete router;

      // 3 rows problem routing with obstruction
      // (can reuse variables that don't change from last time)
      problem = Problem(":/test_problems/3_rows_w_obs.infile");
      router = new Router(problem, settings);
      solve_col = SolveCollection();
      router->routeSuite(problem.pinSets(), problem.cellGrid(), &soft_halt,
          &solve_col);
      // validation
      grid = problem.cellGrid();
      QCOMPARE(grid->allPinsRouted(), false); // impossible problem
      for (int x=0; x<dim_x; x++) {
        for (int y=0; y<dim_y; y++) {
          bool is_routed_cell = grid->cellAt(sp::Coord(x,y))->getType() == sp::RoutedCell;
          QCOMPARE(is_routed_cell, false);  // there must be no routed cells at the end
        }
      }
    }


    //! Test that color generator doesn't crash with the inclusion of more 
    //! colors than the default thresholds.
    void testColorGeneration()
    {
      using namespace settings;

      // make sure the color generation code can generate up to this number of
      // unique colors:
      int max_col = 50; 
      QSet<QString> cols;
      for (int i=0; i<max_col; i++) {
        QColor col =  Settings::colorGenerator(i, max_col);
        QCOMPARE(cols.contains(col.name()), false);  // the new color must not already exist
        cols += col.name();
      }
    }
};

QTEST_MAIN(RouterTests)
#include "pinrouter_tests.moc"  // generated at compile time
