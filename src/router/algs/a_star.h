// @file:     a_star.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     A* algorithm using the Alg base class.


#ifndef _RT_A_STAR_H_
#define _RT_A_STAR_H_

#include "alg.h"
#include "router/routing_records.h"

namespace rt{

  //! A* algorithm class based on the RoutingAlg parent class.
  class AStarAlg : public RoutingAlg
  {
  public:

    //! Empty constructor
    AStarAlg() {};

    //! Empty destructor
    ~AStarAlg() {};

    //! Override the findRoute function to implement the A* algorithm.
    QList<sp::Coord> findRoute(const sp::Coord &source_coord,
        const sp::Coord &sink_coord, sp::Grid *grid, bool routed_cells_lower_cost, 
        bool clear_working_values=true, 
        RoutingRecords *record_keeper=nullptr) override;

  private:

    //! Mark all neighboring cells of a given coordinate by A*. If a valid 
    //! termination is found (whether because it's the sink or because it's a 
    //! routed cell with the same pin_id), then the termination variable is set.
    //! The returned multi map has a pair of ints as keys and eligible 
    //! coordinates as values. The first in the pair of ints represents the 
    //! working value (A* score), the second in the pair has the Manhattan 
    //! distance to the sink (such that closer ones are explored first).
    //! If termination is not sink, then term_to_sink_route ref would be updated
    //! to include the path between the termination and sink.
    QMultiMap<QPair<int,int>,sp::Coord> markNeighbors(const sp::Coord &coord,
        const sp::Coord &source_coord, const sp::Coord &sink_coord, sp::Grid *grid,
        int pin_set_id, bool &marked, QList<sp::Coord> &term_to_sink_route, 
        sp::Coord &termination) const;

    //! Mark neighboring cells contageously from the source coordinate using the
    //! A* algorithm until the specified sink (or eligible routing cell) is 
    //! reached or until no more neighbors are available for marking.
    //! The termination coordinate is set if an eligible sink or routing cell
    //! is found. If termination is not sink, then term_to_sink_route ref would
    //! be updated to include the path between the termination and sink.
    bool runAStar(const sp::Coord &source_coord, const sp::Coord &sink_coord,
        sp::Grid *grid, int pin_set_id, sp::Coord &termination,
        QList<sp::Coord> &term_to_sink_route,
        RoutingRecords *record_keeper=nullptr) const;

    //! Backtrace from the terminating cell recursively. To be called after 
    //! cells have been marked appropriately. Writes route to the route ref.
    void runBacktrace(const sp::Coord &curr_coord, const sp::Coord &source_coord,
        sp::Grid *grid, int pin_set_id, QList<sp::Coord> &route,
        RoutingRecords *record_keeper=nullptr)
      const;

    // Private variables
    bool routed_cells_lower_cost;

  };

}

#endif
