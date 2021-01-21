// @file:     route_graph.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     A simple graph structure for facilitating routing.

#ifndef _RT_ROUTE_GRAPH_H_
#define _RT_ROUTE_GRAPH_H_

#include "spatial.h"

namespace rt {

  //! Forward declarations
  class GNode;
  class GEdge;

  //! Subclass Cell to add convenient routing-related attributes (Route Cell).
  class RCell : sp::Cell
  {
  public:

    //! Set this cell to be a blank cell.
    void setBlankCell();

    //! Set this cell to be a routed cell. Optionally specify which edge or 
    //! node this cell belongs to.
    void setRoutedCell(int pin_set_id, GEdge *e=nullptr, GNode *n=nullptr);

    //! Set the graph edge this cell belongs to.
    void setGEdge(GEdge *e) {g_edge = e; g_node = nullptr;}

    //! Set the graph node this cell represents.
    void setGNode(GNode *n) {g_node = n; g_edge = nullptr;}

    //! Return the graph edge this cell belongs to.
    GEdge *gEdge() {return g_edge;}

    //! Return the graph edge this cell represents.
    GNode *gNode() {return g_node;}


  private:

    // Private variables
    GEdge *g_edge=nullptr;  //!< The graph edge this cell belongs to. nullptr if this is a node.
    GNode *g_node=nullptr;  //!< The graph node this cell represents. nullptr if this is part of an edge.

  };

  //! Subclass Grid to add convenient routing-related attributes (Route Grid).
  class RGrid : sp::Grid
  {

  };

  //! A graph node for routing facilitation.
  class GNode
  {

  };

  //! A graph edge for routing facilitation. Also keeps track of a list of 
  //! RCells associated with this edge.
  class GEdge
  {
  public:

    //! Constructor taking two nodes that defines this edge.
    GEdge(GNode *a, GNode *b);

    //! Replace the connected nodes and update the contained cell list.
    //! Optionally provide the list of RCells this edge should contain if 
    //! already know. Will be automatically determined if not provided.
    void setNodes(GNode *t_a, GNode *t_b, QList<RCell*> t_cells={});

  private:

    // Private variables
    GNode *a;   //!< First node that defines this edge.
    GNode *b;   //!< Second node that defines this edge.

  };

}

#endif
