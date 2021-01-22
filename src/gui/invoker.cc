// @file:     invoker.cc
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Invoker class.

#include "invoker.h"

using namespace gui;

QMap<QString, rt::AvailAlg> Invoker::avail_alg_str;
QMap<QString, rt::LogVerbosity> Invoker::log_vb_str;
QMap<QString, rt::GuiUpdateVerbosity> Invoker::gui_vb_str;

Invoker::Invoker(Viewer *viewer, RouteInspector *inspector, QWidget *parent)
  : QWidget(parent), viewer(viewer), inspector(inspector)
{
  if (avail_alg_str.isEmpty()) {
    initEnumNameMaps();
  }
  initInvoker();
}

void Invoker::setProblem(const rt::Problem &p)
{
  problem = p;
  setEnabled(problem.isValid());  // disable widget if problem is invalid
}

void Invoker::runRoute()
{
  soft_halt=false;
  readRouterSettings();
  inspector->clearCollections();
  if (problem.isValid()) {
    rt::Problem problem_cp(problem);
    rt::Router router(problem_cp, settings);
    rt::RoutingRecords *record_keeper = router.recordKeeper();
    connect(record_keeper, &rt::RoutingRecords::routerStep,
        [this](sp::Grid *grid){viewer->updateCellGrid(grid); qApp->processEvents();});
    /* TODO update to unify
    router.routeSuite(problem_cp.pinSets(), problem_cp.cellGrid(), 
        inspector->solveCollection());
        */
    router.routeSuiteRipReroute(problem_cp.pinSets(), problem_cp.cellGrid(), 
        &soft_halt, inspector->solveCollection());
    inspector->updateCollections();
  } else {
    qDebug() << "Current problem is invalid, not routing.";
  }
}

void Invoker::initEnumNameMaps()
{
  // available algorithms
  avail_alg_str.insert("A*", rt::AStar);
  avail_alg_str.insert("Lee-Moore", rt::LeeMoore);

  // grid logging verbosity
  log_vb_str.insert("Detailed", rt::LogAllIntermediate);
  log_vb_str.insert("Coarse", rt::LogCoarseIntermediate);
  log_vb_str.insert("Results Only", rt::LogResultsOnly);

  // gui update verbosity
  gui_vb_str.insert("Detailed", rt::VisualizeAllIntermediate);
  gui_vb_str.insert("Coarse", rt::VisualizeCoarseIntermediate);
  gui_vb_str.insert("Results Only", rt::VisualizeResultsOnly);
}

void Invoker::initInvoker()
{
  // set this widget to be disabled by default until a valid problem is set
  setEnabled(false);

  // declare GUI elements
  cbb_route_alg = new QComboBox();
  cbb_log_vb = new QComboBox();
  cbb_gui_vb = new QComboBox();
  cb_routed_cells_lower_cost = new QCheckBox();
  cb_net_reordering = new QCheckBox();
  cb_rip_and_reroute = new QCheckBox();
  QPushButton *pb_run = new QPushButton("Route");
  QPushButton *pb_soft_halt = new QPushButton("Soft Halt");
  cbb_route_alg->addItems(avail_alg_str.keys());
  cbb_log_vb->addItems(log_vb_str.keys());
  cbb_gui_vb->addItems(gui_vb_str.keys());

  // set default selections
  cbb_route_alg->setCurrentText(avail_alg_str.key(rt::AStar));
  cbb_log_vb->setCurrentText(log_vb_str.key(rt::LogCoarseIntermediate));
  cbb_gui_vb->setCurrentText(gui_vb_str.key(rt::VisualizeCoarseIntermediate));
  cb_routed_cells_lower_cost->setChecked(true);
  cb_net_reordering->setChecked(true);
  cb_rip_and_reroute->setChecked(true);

  // connect signals
  connect(pb_run, &QPushButton::released, [this](){runRoute();});
  connect(pb_soft_halt, &QPushButton::released, [this](){soft_halt=true;});

  // add everything to widget layout
  QFormLayout *fl_settings = new QFormLayout();
  fl_settings->addRow("Route algorithm", cbb_route_alg);
  fl_settings->addRow("Grid log verbosity", cbb_log_vb);
  fl_settings->addRow("GUI update verbosity", cbb_gui_vb);
  fl_settings->addRow("Routed cells lower cost", cb_routed_cells_lower_cost);
  fl_settings->addRow("Net reordering", cb_net_reordering);
  fl_settings->addRow("Rip and reroute", cb_rip_and_reroute);
  QVBoxLayout *vl_main = new QVBoxLayout();
  vl_main->addLayout(fl_settings);
  vl_main->addWidget(pb_run);
  vl_main->addWidget(pb_soft_halt);
  setLayout(vl_main);
}

void Invoker::readRouterSettings()
{
  settings.use_alg = avail_alg_str[cbb_route_alg->currentText()];
  settings.log_level = log_vb_str[cbb_log_vb->currentText()];
  settings.gui_update_level = gui_vb_str[cbb_gui_vb->currentText()];
  settings.routed_cells_lower_cost = cb_routed_cells_lower_cost->isChecked();
  settings.net_reordering = cb_net_reordering->isChecked();
  settings.rip_and_reroute = cb_rip_and_reroute->isChecked();
}
