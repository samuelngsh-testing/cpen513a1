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
  clearCollections(false);
}

void RouteInspector::clearCollections(bool update_viewer)
{
  solve_col.clear();
  updateCollections();
  if (update_viewer)
    viewer->updateCellGrid();
}

void RouteInspector::updateCollections()
{
  int last_col;
  if (solve_col.solve_steps.size() < 1) {
    // nothing in collection
    last_col = 0;
    g_collection->setEnabled(false);
    pb_show_best->setEnabled(false);
  } else {
    last_col = solve_col.solve_steps.size()-1;
    g_collection->setEnabled(true);
    pb_show_best->setEnabled(true);
  }
  s_collection->setRange(0, last_col);
  s_collection->setValue(last_col);
  updateSteps();
}

void RouteInspector::showBestCollection()
{
  int col_ind = -1;
  int segments = -1;
  int routed_cells = -1;
  for (int i=0; i<solve_col.solve_steps.size(); i++) {
    if (solve_col.solve_steps[i].step_grids.isEmpty()) {
      continue;
    }
    sp::Grid *last_grid = solve_col.solve_steps[i].step_grids.last();
    int t_segments = last_grid->countSegments();
    int t_routed_cells = last_grid->countCells({sp::RoutedCell});
    if (col_ind == -1 || t_segments > segments || 
        (t_segments == segments && t_routed_cells < routed_cells)) {
      qDebug() << tr("Old ind=%1, old segments=%2, old routed_cells=%3").arg(col_ind).arg(segments).arg(routed_cells);
      qDebug() << tr("New ind=%1, new segments=%2, new routed_cells=%3").arg(i).arg(t_segments).arg(t_routed_cells);
      col_ind = i;
      segments = t_segments;
      routed_cells = t_routed_cells;
    }
  }
  showSolveStep(col_ind, -1);
}

void RouteInspector::showSolveStep(int col, int step)
{
  if (col < 0) {
    col = solve_col.solve_steps.size()-1;
  }
  if (step < 0) {
    step = solve_col.solve_steps[col].step_grids.size()-1;
  }
  viewer->updateCellGrid(solve_col[col][step]);
  segments->setText(QString("Segments: %1; routed cells: %2")
      .arg(solve_col[col][step]->countSegments())
      .arg(solve_col[col][step]->countCells({sp::RoutedCell})));
  s_collection->setValue(col);
  s_step->setValue(step);
}

void RouteInspector::showSolveStep()
{
  if (g_collection->isEnabled() && g_step->isEnabled()) {
    showSolveStep(s_collection->value(), s_step->value());
  }
}

void RouteInspector::initInspector()
{
  // init sliders and helper arrow buttons
  segments = new QLabel("Segments: 0; routed cells: 0");
  pb_show_best = new QPushButton("Show Best Collection");
  g_collection = new QGroupBox("Collection");
  g_step = new QGroupBox("Step");
  s_collection = new QSlider(Qt::Horizontal);
  s_step = new QSlider(Qt::Horizontal);
  QPushButton *pb_col_p = new QPushButton(tr("<"));
  QPushButton *pb_col_n = new QPushButton(tr(">"));
  QPushButton *pb_step_p = new QPushButton(tr("<"));
  QPushButton *pb_step_n = new QPushButton(tr(">"));

  // disable the sliders by default (nothing to show yet)
  pb_show_best->setEnabled(false);
  g_collection->setEnabled(false);
  g_step->setEnabled(false);

  // init status layout
  QHBoxLayout *hl_status = new QHBoxLayout();
  hl_status->addWidget(segments);
  hl_status->addWidget(pb_show_best);

  // init collections slider layout
  QHBoxLayout *hl_col = new QHBoxLayout();
  hl_col->addWidget(pb_col_p);
  hl_col->addWidget(s_collection);
  hl_col->addWidget(pb_col_n);
  g_collection->setLayout(hl_col);

  // init steps slider layout
  QHBoxLayout *hl_step = new QHBoxLayout();
  hl_step->addWidget(pb_step_p);
  hl_step->addWidget(s_step);
  hl_step->addWidget(pb_step_n);
  g_step->setLayout(hl_step);

  // add to main layout
  QVBoxLayout *vl_insp = new QVBoxLayout();
  vl_insp->addLayout(hl_status);
  vl_insp->addWidget(g_collection);
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
  connect(pb_show_best, &QAbstractButton::released,
      [this](){showBestCollection();});
  connect(s_collection, &QAbstractSlider::valueChanged,
      this, &RouteInspector::updateSteps);
  connect(s_step, &QAbstractSlider::valueChanged,
      [this](){showSolveStep();});
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
  int orig_val = s_step->value();
  if (!g_collection->isEnabled() || solve_col[curr_col].step_grids.size() == 0) {
    g_step->setEnabled(false);
    s_step->setValue(0);
    segments->setText(QString("Segments: 0; routed cells: 0"));
    return;
  }
  int last_step = solve_col[curr_col].step_grids.size() - 1;
  g_step->setEnabled(true);
  s_step->setRange(0, last_step);
  s_step->setValue(last_step);
  if (orig_val == s_step->value()) {
    // if the value of the slider didn't change, then manually force the new
    // solve step to be shown
    showSolveStep();
  }
}
