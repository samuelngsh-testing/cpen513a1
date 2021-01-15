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

  class Settings
  {
  public:

    static qreal sf;

    static QList<QColor> pin_colors;

  };

}


#endif
