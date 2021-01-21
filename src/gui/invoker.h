// @file:     invoker.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Provides an invocation settings GUI and in charge of invoking 
//            routing as instructed.

#ifndef _GUI_INVOKER_H_
#define _GUI_INVOKER_H_

#include <QtWidgets>
#include "router/problem.h"
#include "router/router.h"
#include "route_inspector.h"

namespace gui{

  class Invoker : public QWidget
  {
    Q_OBJECT

  public:

    //! Constructor taking a pointer to the inspector for direct manipulation.
    Invoker(Viewer *viewer, RouteInspector *inspector, QWidget *parent=nullptr);

    //! Destructor.
    ~Invoker() {};

    //! Set problem.
    void setProblem(const rt::Problem &p);

    //! Create a router and route the problem.
    void runRoute();

  private:

    //! Initialize maps.
    static void initEnumNameMaps();

    //! Initialize the Invoker GUI.
    void initInvoker();

    //! Read current router settings from GUI and update RouterSettings.
    void readRouterSettings();

    // Private variables
    Viewer *viewer;                 //!< Viewer for direct calling.
    RouteInspector *inspector;      //!< Route inspector for direct calling.
    rt::Problem problem;            //!< Current problem.
    rt::RouterSettings settings;    //!< Runtime settings to be sent to router.

    // GUI variables
    QComboBox *cbb_route_alg;
    QComboBox *cbb_log_vb;
    QComboBox *cbb_gui_vb;
    QCheckBox *cb_routed_cells_lower_cost;

    static QMap<QString, rt::AvailAlg> avail_alg_str;
    static QMap<QString, rt::LogVerbosity> log_vb_str;
    static QMap<QString, rt::GuiUpdateVerbosity> gui_vb_str;
  };

}

#endif
