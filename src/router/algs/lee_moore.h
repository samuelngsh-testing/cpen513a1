// @file:     lee_moore.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Lee-Moore algorithm using the Alg base class.


#ifndef _RT_LEE_MOORE_H_
#define _RT_LEE_MOORE_H_

#include "alg.h"
#include "router/routing_records.h"

namespace rt {

  //! Lee-Moore algorithm class based on the RoutingAlg parent class.
  class LeeMooreAlg : public RoutingAlg
  {
  public:

    //! Empty constructor.
    LeeMooreAlg() {};

    //! Empty destructor.
    ~LeeMooreAlg() {};

    //! Override the findRoute function to implement the LeeMoore algorithm.
    bool findRoute(const sp::Coord &source_coord, const sp::Coord &sink_coord,
        sp::Grid *grid, bool routed_cells_lower_cost, 
        RoutingRecords *record_keeper=nullptr) override;

  private:

    //! Mark all neighboring cells of a given coordinate. Returns a list of 
    //! neighbors that were successfully marked.
    QList<sp::Coord> markNeighbors(const sp::Coord &coord, sp::Grid *grid,
        int pin_set_id, bool &marked) const;

    //! Mark neighboring cells contageously from the source coordinate until
    //! the specified sink is reached or until no more neighbors are available
    //! for marking.
    bool runLeeMoore(const sp::Coord &source_coord, const sp::Coord &sink_coord,
        sp::Grid *grid, int pin_set_id, sp::Coord &termination,
        RoutingRecords *record_keeper=nullptr) const;

    //! Backtrace from the sink (or terminating cell) recursively. To be called
    //! after cells have been marked appropriately.
    void runBacktrace(const sp::Coord &curr_coord, const sp::Coord &source_coord,
        const sp::Coord &sink_coord, sp::Grid *grid, int pin_set_id,
        RoutingRecords *record_keeper=nullptr) const;

    // Private variables
    bool routed_cells_lower_cost;
  };

}


#endif
