// @file:     cell.cc
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Cell graphics element class.

#include "cell.h"

using namespace gui;
using namespace settings;

Cell::Cell(sp::Cell *raw_cell)
{
  updateCell(raw_cell);
}

Cell::Cell(sp::CellType type, const sp::Coord &coord, int pin_set_id)
  : pin_set_id(pin_set_id), type(type), coord(coord)
{
  updatePos();
}

void Cell::updateCell(sp::Cell *raw_cell)
{
  pin_set_id = raw_cell->pinSetId();
  type = raw_cell->getType();
  coord = raw_cell->getCoord();
  if (raw_cell->workingValue() > 0)
    misc_text = QObject::tr("%1").arg(raw_cell->workingValue());
  else
    misc_text = "";
  updatePos();
}

QRectF Cell::boundingRect() const
{
  // this cell is bounded by a 1x1 rectangle at the position set in the 
  // constructor
  return QRectF(0, 0, Settings::sf, Settings::sf);
}

void Cell::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
  // TODO this has to change later. Pins alone might have multiple possible
  // colors.
  switch(type) {
    case sp::PinCell:
    case sp::RoutedCell:
      col = Settings::pin_colors[pin_set_id];
      break;
    case sp::ObsCell:
      col = QColor("#0000FF");
      break;
    case sp::BlankCell:
    default:
      col = QColor("#FFFFFF");
      break;
  }
  // draw rectangle
  painter->setBrush(col);
  painter->drawRect(boundingRect());
  // draw set ID if is a Pin
  if (type == sp::PinCell) {
    painter->setBrush(QColor("#000000"));
    painter->drawText(boundingRect(), Qt::TopLeftCorner, QObject::tr("S%1").arg(pin_set_id));
  }
  if (!misc_text.isEmpty()) {
    painter->setBrush(QColor("#000000"));
    painter->drawText(boundingRect(), Qt::AlignCenter, misc_text);
  }
}

void Cell::updatePos()
{
  // position denotes the top left position of the cell
  setPos(coord.x*Settings::sf, coord.y*Settings::sf);
}
