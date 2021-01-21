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
#include "routing_records.h"
#include "algs/alg.h"
#include "algs/a_star.h"
#include "algs/lee_moore.h"

// router namespace
namespace rt {

  //! Store router settings
  struct RouterSettings
  {
    // routing settings
    AvailAlg use_alg=AStar;
    bool routed_cells_lower_cost=false;

    // verbosity settings
    LogVerbosity detail_level=LogCoarseIntermediate;
    GuiUpdateVerbosity gui_update_level=VisualizeCoarseIntermediate;
  };

  //! A router attempts to create connections between all pins in a provided 
  //! problem. If that is not possible, then it aims to connect as many of them 
  //! as possible by various heuristics.
  class Router : public QObject
  {
  Q_OBJECT

  public:
    //! Constructor for the Router class taking the problem class and cache path.
    Router(const Problem &problem, RouterSettings settings);

    //! Destructor.
    ~Router() {};

    //! Return a pointer to the current record keeping helper class.
    RoutingRecords *recordKeeper() {return records;}

    //! Attempt to route everything. Returns whether all have been successfully
    //! routed.
    bool routeSuite(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid, 
        SolveCollection *solve_col);

  private:

    // Private variables
    RoutingRecords *records;  //!< class that keeps record of routing progress
    Problem problem;          //!< the problem to be routed
    RouterSettings settings;  //!< router settings

  };

}

#endif
