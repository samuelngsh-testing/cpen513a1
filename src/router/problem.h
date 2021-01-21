// @file:     problem.h
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     The problem to be routed.

#ifndef _RT_PROBLEM_H_
#define _RT_PROBLEM_H_

#include <QObject>
#include <QMap>
#include <algorithm>
#include "spatial.h"

// router namespace
namespace rt {


  //! A routing problem to be routed. Contains the problem dimensions, various
  //! collections of cells, etc.
  class Problem
  {
  public:
    //! Stages when reading the input file.
    enum ReadPhase{GridSize, ObsCellCount, ObsCells, PinCount, Pins, Finished};
    
    //! Constructor for a problem to be routed, taking the problem file as input.
    Problem(const QString &in_path="");

    //! Copy constructor
    Problem(const Problem &other) 
      : dim_x(other.dim_x), dim_y(other.dim_y), obs_cells(other.obs_cells), 
        pin_sets(other.pin_sets) {refreshGrid();}

    //! Destructor.
    ~Problem() {};

    //! Read the problem from the input path. Return true if successful, false
    //! otherwise (e.g. if input file contains invalid formatting).
    bool readProblem(const QString &in_path);

    //! Return whether this problem is valid. Invalid if there are no pins at 
    //! all, if pins/obstruction cells exist outside of the specified 
    //! x and y dimensions, or if there are overlaps between pins and 
    //! obstruction cells.
    bool isValid() const;

    //! Return the dimensions of the problem as a Coord(dim_x, dim_y). There 
    //! should be cells in the ranges [0,dim_x) and [0,dim_y) in the x and y 
    //! directions.
    sp::Coord dimensions() const {return sp::Coord(dim_x, dim_y);}

    //! Return a pointer to the cell grid
    sp::Grid *cellGrid() {return &cell_grid;}

    //! Return a list of pin sets.
    QList<sp::PinSet> pinSets() const {return pin_sets;}

  private:

    //! Construct cell map.
    void refreshGrid();

    // Private variables:
    int dim_x=-1, dim_y=-1;       //!< x and y dimensions.
    QList<sp::Coord> obs_cells;   //!< List of obstruction cells.
    QList<sp::PinSet> pin_sets;   //!< List of sets of pins
    sp::Grid cell_grid;           //!< Grid of cells in the problem
  };
}

#endif
