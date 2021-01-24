// @file:     problem_tests.cpp
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Unit tests that ensure the Problem class reads a known problem 
//            file properly into the needed lists and maps.

#include <QtTest/QtTest>
#include "router/problem.h"

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

    //! Test the capability to read problems and create appropriate data structures
    void testProblemRead()
    {
      using namespace rt;

      // a minimal problem for testing
      // Looks like:
      // P         P
      // TODO change to three rows and put the right pin low
      // TODO add pin set with the three row version
      // (9 blank spaces)
      Problem problem(":/test_problems/straight_line.infile");
      QCOMPARE(problem.isValid(), true);
      // x and y dimensions
      QCOMPARE(problem.dimensions().x, 11);
      QCOMPARE(problem.dimensions().y, 1);
      QList<sp::Coord> obs;   // no obstruction cells for this problem
      QList<sp::Coord> pins({sp::Coord(0,0), sp::Coord(10,0)});
      QList<sp::Coord> blanks;
      for (int x=1; x<10; x++) {
        blanks.append(sp::Coord(x,0));
      }
      sp::Grid *grid = problem.cellGrid();
      QCOMPARE(checkTypeOfCells(obs, grid, sp::ObsCell), true);
      QCOMPARE(checkTypeOfCells(pins, grid, sp::PinCell, 0), true);
      QCOMPARE(checkTypeOfCells(blanks, grid, sp::BlankCell), true);
      grid = nullptr;

      // similar problem as above but with obstruction cells
      // Looks like:
      // P   OOO   P
      // (3 obstruction cells in the middle)
      problem = Problem(":/test_problems/straight_line_w_obs.infile");
      QCOMPARE(problem.isValid(), true);
      // x and y dimensions
      QCOMPARE(problem.dimensions().x, 11);
      QCOMPARE(problem.dimensions().y, 1);
      obs = QList<sp::Coord>({sp::Coord(4,0), sp::Coord(5,0), sp::Coord(6,0)});
      pins = QList<sp::Coord>({sp::Coord(0,0), sp::Coord(10,0)});
      blanks = QList<sp::Coord>({sp::Coord(1,0), sp::Coord(2,0), sp::Coord(3,0),
          sp::Coord(7,0), sp::Coord(8,0), sp::Coord(9,0)});
      grid = problem.cellGrid();
      QCOMPARE(checkTypeOfCells(obs, grid, sp::ObsCell), true);
      QCOMPARE(checkTypeOfCells(pins, grid, sp::PinCell, 0), true);
      QCOMPARE(checkTypeOfCells(blanks, grid, sp::BlankCell), true);
    }

    //! Test the capability to route the problems above (or appropriately fail for
    //! unroutable problems)
    // TODO
    void testProblemRoute()
    {
      // TODO for the routable 2 row problem, check that each column contains 
      // at least 1 conductor. At least one of those columns must contain two
      // conductors (pins also count)
    }


    // TODO test that color generator doesn't crash with the inclusion of more colors (than the default thresholds)

};

QTEST_MAIN(RouterTests)
#include "pinrouter_tests.moc"  // generated at compile time
