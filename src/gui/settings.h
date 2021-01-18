// @file:     settings.cc
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Convenient settings.

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QObject>
#include <QColor>

namespace settings {

  //! Class containing handy settings.
  class Settings
  {
  public:

    //! Graphics viewer scaling factor (how many pixels per grid cell).
    static qreal sf;

    //! Return a color that generated as suitable for the provided index and 
    //! max possible index.
    static QColor colorGenerator(int ind, int max_ind);

    //! Generated colors.
    static QList<QColor> gcols;

  };

}


#endif
