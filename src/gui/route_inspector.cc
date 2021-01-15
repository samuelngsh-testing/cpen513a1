// @file:     route_inspector.cc
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the route inspector class.

#include "route_inspector.h"
#include <QPushButton>

using namespace gui;
using namespace settings;

RouteInspector::RouteInspector(Viewer *viewer, QWidget *parent)
  : QWidget(parent), viewer(viewer)
{
  initInspector();
}

RouteInspector::~RouteInspector()
{
  clearCollections();
}

void RouteInspector::clearCollections()
{
  solve_col.clear();
  updateCollections();
  viewer->updateCellGrid();
}

void RouteInspector::updateCollections()
{
  int last_col;
  if (solve_col.solve_steps.size() < 1) {
    // nothing in collection
    last_col = 0;
    g_collection->setEnabled(false);
    qDebug() << "Nothing in collection, slider disabled.";
  } else {
    last_col = solve_col.solve_steps.size()-1;
    g_collection->setEnabled(true);
  }
  s_collection->setRange(0, last_col);
  s_collection->setValue(last_col);
  updateSteps();
}

void RouteInspector::showSolveStep(int col, int step)
{
  if (col < 0) {
    col = solve_col.solve_steps.size()-1;
  }
  if (step < 0) {
    step = solve_col.solve_steps[step].step_grids.size()-1;
  }
  viewer->updateCellGrid(solve_col[col][step]);
}

void RouteInspector::initInspector()
{
  // init sliders and helper arrow buttons
  g_collection = new QGroupBox("Collection");
  g_step = new QGroupBox("Step");
  s_collection = new QSlider(Qt::Horizontal);
  s_step = new QSlider(Qt::Horizontal);
  QPushButton *pb_col_p = new QPushButton(tr("<"));
  QPushButton *pb_col_n = new QPushButton(tr(">"));
  QPushButton *pb_step_p = new QPushButton(tr("<"));
  QPushButton *pb_step_n = new QPushButton(tr(">"));

  // disable the sliders by default (nothing to show yet)
  g_collection->setEnabled(false);
  g_step->setEnabled(false);

  // init layout
  QVBoxLayout *vl_insp = new QVBoxLayout();
  vl_insp->addWidget(new QLabel("Collection:"));
  QHBoxLayout *hl_col = new QHBoxLayout();
  hl_col->addWidget(pb_col_p);
  hl_col->addWidget(s_collection);
  hl_col->addWidget(pb_col_n);
  g_collection->setLayout(hl_col);
  vl_insp->addWidget(g_collection);

  vl_insp->addWidget(new QLabel("Step:"));
  QHBoxLayout *hl_step = new QHBoxLayout();
  hl_step->addWidget(pb_step_p);
  hl_step->addWidget(s_step);
  hl_step->addWidget(pb_step_n);
  g_step->setLayout(hl_step);
  vl_insp->addWidget(g_step);

  // lambda function to deal with adding or substracting from a particular slider
  auto moveSlider = [](QSlider *s, int change)
  {
    int final_ind = s->value() + change;
    if (final_ind <= s->maximum() && final_ind >= s->minimum()) {
      s->setValue(final_ind);
    }
  };

  // connect signals
  connect(s_collection, &QAbstractSlider::valueChanged,
      this, &RouteInspector::updateSteps);
  connect(s_step, &QAbstractSlider::valueChanged,
      [this]()
      {
        if (g_collection->isEnabled() && g_step->isEnabled()) {
          showSolveStep(s_collection->value(), s_step->value());
        }
      });
  connect(pb_col_p, &QAbstractButton::released,
      [this, moveSlider](){moveSlider(s_collection, -1);});
  connect(pb_col_n, &QAbstractButton::released,
      [this, moveSlider](){moveSlider(s_collection, 1);});
  connect(pb_step_p, &QAbstractButton::released,
      [this, moveSlider](){moveSlider(s_step, -1);});
  connect(pb_step_n, &QAbstractButton::released,
      [this, moveSlider](){moveSlider(s_step, 1);});
  setLayout(vl_insp);
}

void RouteInspector::updateSteps()
{
  int curr_col = s_collection->value();
  if (!g_collection->isEnabled() || solve_col[curr_col].step_grids.size() == 0) {
    g_step->setEnabled(false);
    s_step->setValue(0);
    qDebug() << "Nothing in routing steps, slider disabled.";
    return;
  }
  int last_step = solve_col[curr_col].step_grids.size() - 1;
  g_step->setEnabled(true);
  s_step->setRange(0, last_step);
  s_step->setValue(last_step);
}
