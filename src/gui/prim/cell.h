// @file:     cell.h
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     The Cell class is a primitive graphical element that displays 
//            an individual grid cell on the problem grid.

#ifndef _GUI_CELL_H_
#define _GUI_CELL_H_

#include <QtWidgets>
#include "router/problem.h"
#include "gui/settings.h"

namespace gui {
  
  //! A primitive graphical element that displays an individual grid cell on the
  //! problem grid. Can be used to represent pins, obstruction cells, routed
  //! cells, or blank space. The problem grid is filled with these cells.
  class Cell : public QGraphicsItem
  {
  public:

    //! Constructor taking a spatial Cell pointer to instantiate the graphical
    //! cell.
    Cell(sp::Cell *raw_cell);

    //! Constructor taking the default cell role (which can later be changed)
    //! and its coordinate.
    Cell(sp::CellType type, const sp::Coord &coord, int pin_set_id=-1);

    //! Update cell with the provided sp::Cell info.
    void updateCell(sp::Cell *raw_cell);

    //! Switch the cell type to the specified type.
    void setType(sp::CellType t_type) {type = t_type;}

    //! Return the current cell type.
    sp::CellType getType() const {return type;}

    //! Set the cell coordinates.
    void setCoord(const sp::Coord &t_coord) {coord = t_coord; setPos(coord.x, coord.y);}

    //! Return the cell coordinates.
    sp::Coord getCoord() const {return coord;}

    //! Overriden method to return the proper bounding rectangle of this cell.
    virtual QRectF boundingRect() const override;

    //! Overriden method to paint this cell on scene.
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;

    //! Static number of sets of pins of the problem currently displayed.
    static int num_pin_sets;

  private:

    //! Update the cell's graphical location
    void updatePos();

    // Private variables
    int pin_set_id;         //!< The set of pins this cell belongs to, -1 if not a pin.
    sp::CellType type;      //!< The type of cell this object represents.
    sp::Coord coord;        //!< The coordinate of this cell.
    QColor col;             //!< The color this cell should be.
    QString misc_text;      //!< Misc text to be shown in the cell.

  };
}

#endif
