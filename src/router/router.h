// @file:     router.h
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     Generate routings for the specified problem file.

#ifndef _RT_ROUTER_H_
#define _RT_ROUTER_H_

#include <QObject>
#include "problem.h"

// router namespace
namespace rt {

  //! Store information related to a solve attempt within a collection.
  struct SolveSteps
  {
    ~SolveSteps() {step_grids.clear();}

    //! Convenient index operator to access a step grid.
    sp::Grid *operator[](int i) {return step_grids.value(i);}

    QString desc;                 //!< Description of this attempt.
    QList<sp::Grid*> step_grids;  //!< A list of grids containing every step in the solve attempt
  };

  //! Store information on a collection of solve attempts.
  struct SolveCollection
  {
    //! Clear the collection
    void clear() {desc = ""; solve_steps.clear();}

    //! Convenient index operator to access a particular collection of SolveSteps
    SolveSteps operator[](int i) {return solve_steps.value(i);}

    //! Add a new SolveSteps object and return its pointer
    SolveSteps *newSolveSteps()
    {
      solve_steps.append(SolveSteps());
      return &solve_steps.back();
    }
    
    QString desc;                   //!< Description of this collection.
    QList<SolveSteps> solve_steps;  //!< A list of solve steps.
  };

  class Router : public QObject
  {
  public:
    //! Supported routing algorithms
    enum RouteAlg{LeeMoore, AStar};

    //! Constructor for the Router class taking the problem class and cache path.
    Router(const Problem &problem, const QString &cache_path);

    //! Destructor.
    ~Router() {};

    //! Route the wire for the specified set of pins on the provided cell grid.
    //! The success state is written to the bool references.
    bool routeForId(RouteAlg alg, sp::PinSet pin_set, sp::Grid *cell_grid,
        SolveCollection *solve_col=nullptr);

    //! Run the Lee Moore algorithm from the specified source to the specified
    //! sink. If use_routed_cell_from_same_set is set to true, then existing 
    //! routed cells that belong to the same set of pins are used.
    bool leeMoore(const sp::Coord &source_coord, const sp::Coord &sink_coord, 
        sp::Grid *cell_grid, SolveSteps *solve_steps=nullptr);

  private:

    // Private variables
    Problem problem;      //!< the problem to be routed
    QString cache_path;   //!< path to the cache directory

  };

}

#endif
