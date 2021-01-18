// @file:     settings.h
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Settings implementations.

#include <QtMath>
#include <QDebug>
#include "settings.h"

using namespace settings;

qreal Settings::sf = 50;

QList<QColor> Settings::gcols;

QColor Settings::colorGenerator(int ind, int max_ind)
{
  // generate color list if it's empty or if the max index has changed beyond
  // what's currently supported by the existing list
  if (gcols.isEmpty() || max_ind > gcols.size()) {
    gcols.clear();

    // define a color tier threshold where the generation algorithm changes a bit
    // depending on the max_ind provided
    QList<int> col_tier_thresh = {0, 6, 9, 12};
    QList<qreal> h_fact = {0.83, 0.5, 0.5};
    QList<qreal> l_fact = {0.75, 0.5, 0.25};

    // scale the color tier thresholds if max_ind is greater than the greatest threshold
    if (max_ind > col_tier_thresh.last()) {
      qreal scale_fact = (qreal)max_ind/col_tier_thresh.last();
      for (int &thresh : col_tier_thresh) {
        thresh = qCeil((qreal) thresh * scale_fact);
      }
    }

    // generate the color list
    for (int i=0; i<col_tier_thresh.last(); i++) {
      // choose color based on the given index and max ind
      // The HSL color space is used
      qreal h, l;
      qreal s = 1.0;
      // step through the color tiers
      for (int j=1; j<col_tier_thresh.size(); j++) {
        int prev_thresh = col_tier_thresh[j-1];
        int thresh = col_tier_thresh[j];
        if (i < thresh) {
          int tier_ind = col_tier_thresh.indexOf(thresh) - 1;
          h = h_fact[tier_ind] * (qreal)(i-prev_thresh)/thresh;
          l = l_fact[tier_ind];
          break;
        }
      }
      QColor col;
      col.setHslF(h, s, l);
      gcols.append(col);
    }
  }
  return gcols[ind];
}
