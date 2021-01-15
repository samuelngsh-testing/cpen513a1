// @file:     settings.h
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Settings implementations.

#include "settings.h"

using namespace settings;

qreal Settings::sf = 50;

// TODO generate colors instead of hard-coding selections
QList<QColor> Settings::pin_colors({QColor("#FF0000"), QColor("#00FF00"), 
    QColor("#FFFF00"), QColor("#00FFFF"), QColor("#FF00FF"),
    QColor("#FF6600"), QColor("#0066FF"), QColor("#6600FF"),
    QColor("#66FF00")});
