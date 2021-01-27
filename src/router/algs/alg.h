// @file:     alg.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Base class for a router algorithm.

#ifndef _RT_ALG_H_
#define _RT_ALG_H_

#include "router/routing_records.h"

// router namespace
namespace rt {

  enum AvailAlg{LeeMoore, AStar};

  //! A struct for returning routes to caller.
  struct RouteResult {
    QList<sp::Coord> route_coords;
    bool requires_rip;
  };

  //! A base class for routing algorithms.
  class RoutingAlg
  {
  public:
    //! Empty constructor.
    RoutingAlg() {};

    //! Empty destructor.
    virtual ~RoutingAlg() {};

    //! Attempt to find a route between the provided source and sink coordinates.
    //! Return the list of coordinates that represent a route.
    //! If a route is impossible without ripping but possible with, then rip_cand
    //! is set to contain candidates to rip in order to allow a connection.
    //! If rip_cand is nullptr then no such ripping would be considered.
    //! (For subclass to implement.)
    virtual RouteResult findRoute(const sp::Coord &source_coord, 
        const sp::Coord &sink_coord, sp::Grid *grid, bool routed_cells_lower_cost, 
        bool clear_working_values=true, bool attempt_rip=false,
        QList<sp::Connection*> *rip_blacklist=nullptr,
        RoutingRecords *record_keeper=nullptr) = 0;

  };

}

#endif
