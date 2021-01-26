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
    AvailAlg use_alg=AStar;             //!< routing algorithm to use
    bool routed_cells_lower_cost=false; //!< existing routes have lower traverse cost
    bool net_reordering=true;           //!< enable net reordering
    int difficult_boost_thresh=2;       //!< boost the order of a difficult route after failing this many times
    int max_rerun_count=5;              //!< maximum global reroute count (with net reordering)
    bool rip_and_reroute=true;          //!< enable rip and reroute
    int rip_and_rerout_count=2;         //!< maximum rip and reroute attempts for a route

    // verbosity settings
    LogVerbosity log_level=LogCoarseIntermediate;
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

    //! Attempt to route with rip and reroute (TODO after everything is settled,
    //! put rip and reroute and net reordering as configurable settings)
    bool routeSuiteRipReroute(QList<sp::PinSet> pin_sets, sp::Grid *cell_grid, 
        bool *soft_halt, SolveCollection *solve_col);

    //! Create a routed connection with the provided list of coordinates and 
    //! settings.
    sp::Connection *createConnection(const sp::PinPair &pin_pair,
        const QList<sp::Coord> &route, int pin_set_id, sp::Grid *grid,
        RoutingRecords *record_keeper=nullptr);

    //! Return a set of connections that the provided list of coordinates 
    //! cross through (empty list if there are no existing connections).
    QSet<sp::Connection*> existingConnections(const QList<sp::Coord> &coords,
        sp::Grid *grid, int ignore_pin_id=-1);

    //! Rip a connection (turn routed cells to blank). If a cell is used for 
    //! more than one connection, it would not be ripped (but the specified 
    //! Connection record would still be destroy).
    //! This does not delete the Connection pointer, the caller should deal with
    //! that.
    void ripConnection(sp::Connection *conn, sp::Grid *grid,
        RoutingRecords *record_keeper=nullptr);

  private:

    //! Prepare variables before routing.
    void routePrep(const QList<sp::PinSet> &pin_sets, 
        QMultiMap<int,sp::PinPair> &map_pin_sets, QSet<sp::Coord> &unrouted_pins,
        RoutingAlg **alg);

    //! Go through a routine that attempts to route the source to the sink.
    //! If the route is only available by rip and reroute and if it is allowed,
    //! attempt rip and reroute. Returns whether it is successful or not.
    bool routePinPair(RoutingAlg *alg, const sp::PinPair &pin_pair, sp::Grid *grid);

    // Private variables
    RoutingRecords *records;  //!< class that keeps record of routing progress
    Problem problem;          //!< the problem to be routed
    RouterSettings settings;  //!< router settings

  };

}

#endif
