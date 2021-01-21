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

  //! A base class for routing algorithms.
  class RoutingAlg
  {
  public:
    //! Empty constructor.
    RoutingAlg() {};

    //! Empty destructor.
    virtual ~RoutingAlg() {};

    //! Attempt to find a route between the provided source and sink coordinates.
    //! Return whether or not a connection was made.
    //! (For subclass to implement.)
    virtual bool findRoute(const sp::Coord &source_coord, const sp::Coord &sink_coord,
        sp::Grid *grid, bool routed_cells_lower_cost, 
        RoutingRecords *record_keeper=nullptr) = 0;

  };

}

#endif
