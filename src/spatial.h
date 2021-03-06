// @file:     spatial.h
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Spatial classes (e.g. coordinates, grids, etc.)

#ifndef _SP_SPATIAL_H_
#define _SP_SPATIAL_H_

#include <QVariant>
#include <QObject>
#include <QVector>
#include <QMap>
#include <functional>

// spatial namespace
namespace sp {

  //! Cell type specifier at each grid location.
  enum CellType{PinCell, ObsCell, RoutedCell, BlankCell};

  //! A coordinate with x and y components and some helpful functions.
  class Coord
  {
  public:
    //! Constructor taking the x and y coordinates.
    Coord(int x, int y) : x(x), y(y), is_blank(false) {};

    //! Empty coordinate.
    Coord() : x(-1), y(-1), is_blank(true) {};

    //! Copy constructor.
    //Coord(const Coord &other) : x(other.x), y(other.y), is_blank(other.is_blank) {};

    //! Return whether this is a blank coordinate.
    bool isBlank() const {return is_blank;}

    //! == operator
    bool operator==(const Coord &other) const;

    //! != operator
    bool operator!=(const Coord &other) const {return !((*this)==other);}

    //! Return the coordinates in parenthesis string form for easy debugging.
    QString str() const {return QString("(%1, %2)").arg(x).arg(y);}

    //! Return the Manhattan distance between this Coord and another one.
    int manhattanDistance(const Coord &other) const;

    //! Return the Manhattan distance between any two Coord objects.
    static int manhattanDistance(const Coord &a, const Coord &b)
    {
      return a.manhattanDistance(b);
    }

    //! Return whether this Coord is within the given grid bounds. Returns true
    //! if x falls within [0,x_max) and for y [0,y_max).
    bool isWithinBounds(int x_max, int y_max) const;

    //! Return the coordinate to the left of this cell.
    Coord left() const {return Coord(x-1, y);}

    //! Return the coordinate to the right of this cell.
    Coord right() const {return Coord(x+1, y);}

    //! Return the coordinate above this cell (assuming y increases downwards).
    Coord above() const {return Coord(x, y-1);}

    //! Return the coordinate below this cell (assuming y increases downwards).
    Coord below() const {return Coord(x, y+1);}

    // Public variables:
    int x;          //!< x coordinate
    int y;          //!< y coordinate
    bool is_blank;  //!< specify whether this coordinate contains real info
  };

  //! operator needed for Coord to be used as keys in QMap
  inline bool operator<(const Coord &c1, const Coord &c2)
  {
    // exploit Qt's built in hashing for QPairs
    uint h1 = qHash(QPair<int,int>(c1.x, c1.y));
    uint h2 = qHash(QPair<int,int>(c2.x, c2.y));
    return (h1 < h2);
  }

  //! Declare PinSet as an alias that stores sets of pins to be connected
  using PinSet = QList<Coord>;

  //! Declare PinPair as an alias that stores pairs of pins (useful for hashing)
  using PinPair = QPair<sp::Coord, sp::Coord>;

  //! Handy class for keeping track of routed wires.
  class Connection
  {
  public:

    //! Empty constructor.
    Connection() : pin_set_id(-1) {};

    //! Construct with provided cells and pin set ID.
    Connection(PinPair t_pin_pair, const QList<sp::Coord> &coords, int t_pin_set_id)
    {
      setRoutedCells(t_pin_pair, coords, t_pin_set_id);
    }

    //! Copy constructor given a pointer to another conneciton.
    Connection(Connection *other) : pin_pair(other->pin_pair), 
      pin_set_id(other->pin_set_id), routed_cells(other->routed_cells) {};

    //! Set the provided coordinates to belong to this connecction.
    void setRoutedCells(const PinPair &t_pin_pair, const QList<sp::Coord> &cells,
        int t_pin_set_id)
    {
      pin_pair = t_pin_pair;
      routed_cells = cells;
      pin_set_id = t_pin_set_id;
    }

    //! Return the pin set id
    int pinSetId() const {return pin_set_id;}

    //! Return the routed cells that belong to this connection.
    QList<sp::Coord> routedCells() const {return routed_cells;}

    //! Return the pair of pins this route connects.
    PinPair pinPair() {return pin_pair;}

    //! Return whether this is connection is empty or not.
    bool isEmpty() {return routed_cells.isEmpty();}

  private:

    // Private variables
    PinPair pin_pair;               //! The pair of pins that this connection is for
    int pin_set_id = -1;            //! Pin set ID
    QList<sp::Coord> routed_cells;  //! A list of routed cells that belong to this connection
  };

  //! A cell that belongs to a grid data structure.
  class Cell
  {
  public:
    //! Constructor taking the coordinates.
    Cell(const Coord &coord, CellType type, int pin_set_id=-1)
      : coord(coord), type(type), pin_set_id(pin_set_id) {};

    //! Copy constructor
    Cell(const Cell &other)
      : coord(other.coord), type(other.type), pin_set_id(other.pin_set_id),
        working_val(other.working_val), extra_props(other.extra_props) {};

    //! Empty constructor defaulting to blank cell.
    Cell() : coord(), type(BlankCell) {};

    //! Destructor.
    ~Cell() {};

    //! Set the coordinates of this cell.
    void setCoord(const Coord &t_coord) {coord = t_coord;}

    //! Return the coordinates of this cell.
    Coord getCoord() const {return coord;}

    //! Set the type of this cell.
    void setType(CellType t_type) {type = t_type;}

    //! Get the type of this cell.
    CellType getType() const {return type;}

    //! Set the pin set ID of this cell (if this belongs to a pin set or a 
    //! routed wire).
    void setPinSetId(int t_id) {pin_set_id = t_id;}

    //! Return the pin set ID.
    int pinSetId() const {return pin_set_id;}

    //! Set the working value associated with this cell (intended for storing 
    //! misc routing information when it's in progress).
    void setWorkingValue(int val) {working_val = val;}

    //! Reset the working value to some default invalid value (-1).
    void resetWorkingValue() {working_val = -1;}

    //! Return the working value.
    int workingValue() const {return working_val;}

    //! Return a pointer to the extra_props map (intended for routing algorithms
    //! to store extra properties.)
    QMap<QString, QVariant> &extraProps() {return extra_props;}

  private:

    // Private variables
    Coord coord;          //!< Coordinates of this cell.
    CellType type;        //!< The type of this cell.
    int pin_set_id=-1;    //!< The pin set this belongs if (if it's a pin). Not a pin if -1.
    int working_val=-1;   //!< Misc value intended for storing routing information.
    QMap<QString, QVariant> extra_props;  //!< A map of extra properties.
  };

  //! A 2D grid containing the problem
  class Grid
  {
  public:
    //! Constructor taking the grid size, obstructions and pins. This is meant
    //! to construct a grid that has not been solved at all.
    Grid(int dim_x, int dim_y, const QList<Coord> &obs_coords={}, 
        const QList<PinSet> &pin_sets={});

    //! Copy constructor but clones the cell grid rather than using the same
    //! pointer.
    Grid(Grid *other);

    //! Constructor for an empty grid.
    Grid() : dim_x(0), dim_y(0) {};

    //! Destructor.
    ~Grid();

    //! Set all grid cells to become identical to the given grid.
    void copyState(Grid *other);

    //! Set the dimensions of the grid.
    void setGridSize(int x, int y) {dim_x = x; dim_y=y;}

    //! Set obstruction cells.
    void setObsCells(const QList<Coord> &obs_coords, bool check_clash=false);

    //! Set pin cells with the given pin set ID.
    void setPinCells(const QList<Coord> &pin_coords, int pin_set_id,
        bool check_clash=false);

    //! Index operator.
    Cell *operator()(int x, int y) {return cell_grid.value(x).value(y);}

    //! Return cell at the specified coordinate.
    Cell *operator()(const Coord &coord) {return cellAt(coord);}

    //! Return cell at the specified coordinate.
    Cell *cellAt(const Coord &coord) {return cell_grid.value(coord.x).value(coord.y);}

    //! Return a list cells that are neighbors of the provided coordinate,
    //! excluding out of bound coordinates.
    QList<Cell*> neighborsOf(const Coord &coord);

    //! Return a list of neighboring coordinates of the provided coordinate,
    //! excluding out of bound coordinates.
    QList<Coord> neighborCoordsOf(const Coord &coord);

    //! Return a pointer to the cell grid.
    QVector<QVector<Cell*>> *cellGrid() {return &cell_grid;}

    //! Return a MultiMap that contains the connection attributes with 
    //! coordinates as keys.
    QMultiMap<sp::Coord,Connection*> *connMap() {return &conn;}

    //! Clear all working values from all cells in the grid.
    void clearWorkingValues();

    //! Return whether the specified coordinates are within bounds.
    bool isWithinBounds(const Coord &);

    //! Return whether a route exists between the provided pins. If a route list
    //! is provided and if a route does exist, coordinates constituting the 
    //! route would be added to the list pointer.
    //! Note that the points a and b will NOT be added to the route pointer.
    bool routeExistsBetweenPins(const Coord &a, const Coord &b, 
        QList<sp::Coord> *route=nullptr);

    //! Return all pins that a specified coordinate is connected to (via routed
    //! cells or pins of the same pin set id). If the provided coordinate is 
    //! not part of a wire, then the returned list is blank.
    QList<Coord> connectedPins(const Coord &coord);

    //! Return whether all pins have been connected by RoutedCells. Checks by 
    //! exhaustively tracing all pin combinations of each pin set.
    bool allPinsRouted();

    //! Return the count of connected segments.
    int countSegments();

    //! Count cells of the specified types. If the provided list is empty, 
    //! returns a count of all cells (just width * height).
    int countCells(const QSet<CellType> &types);


  private:

    //! Clear all cells
    void clearGrid();

    // Private variables
    int dim_x;                              //!< x size.
    int dim_y;                              //!< y size.
    QVector<QVector<Cell*>> cell_grid;      //!< Grid of cells.
    QMap<int,PinSet> pin_sets;              //!< Keep track of pin sets.
    QMultiMap<sp::Coord,Connection*> conn;  //!< Keep track of pin pair connections.
  };

  inline uint qHash(const Coord &coord, uint seed=0)
  {
    return ::qHash(coord.x, seed) + ::qHash(coord.y, seed);
  }

}


Q_DECLARE_METATYPE(sp::Coord);

#endif
