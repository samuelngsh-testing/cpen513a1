// @file:     mainwindow.h
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     The MainWindow class containing relevant widgets for the routing 
//            GUI.

#ifndef _GUI_MAINWINDOW_H_
#define _GUI_MAINWINDOW_H_

#include <QtWidgets>
#include "router/problem.h"
#include "router/router.h"
#include "viewer.h"
#include "route_inspector.h"
#include "invoker.h"

namespace gui {

  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:

    //! Constructor taking the input file path describing the problem to be 
    //! routed.
    explicit MainWindow(const QString &in_path, QWidget *parent=nullptr);

    //! Destructor (contained widgets are automatically destroyed by Qt).
    ~MainWindow() {};

    //! Show the About dialog.
    void aboutDialog();

    //! Override the resize event to fit the problem in the viewport when user
    //! resizes the main window.
    virtual void resizeEvent(QResizeEvent *) override;

    //! Read a problem file and show it in the viewer.
    void readAndShowProblem(const QString &in_path);

  private:

    //! Initialize the GUI.
    void initGui();

    //! Initialize the top menu bar.
    void initMenuBar();

    // Private variables
    rt::Problem problem;    //!< Problem to be routed.
    gui::Viewer *viewer;    //!< Pointer to the problem viewer.
    gui::RouteInspector *inspector; //!< Pointer to the route inspector.
    gui::Invoker *invoker;  //!< Pointer to router Invoker.
    QString open_dir_path;  //!< Store the directory path of the last opened file.

  };

}

#endif
