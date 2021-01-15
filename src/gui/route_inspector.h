// @file:     route_inspector.h
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     RouteInspector provides a GUI to view detail routing steps.

#ifndef _GUI_ROUTE_INSPECTOR_H_
#define _GUI_ROUTE_INSPECTOR_H_

#include <QtWidgets>
#include "router/problem.h"
#include "router/router.h"
#include "viewer.h"

namespace gui {

  class RouteInspector : public QWidget
  {
    Q_OBJECT

  public:

    //! Constructor taking a pointer to the viewer for direct manipulation.
    RouteInspector(Viewer *viewer, QWidget *parent=nullptr);

    //! Destructor.
    ~RouteInspector();

    //! Return a pointer to SolveCollections. This is intended to allow the 
    //! router to write solve steps directly to it.
    rt::SolveCollection *solveCollection() {return &solve_col;}

    //! Clear the existing collection.
    void clearCollections();

    //! Update the inspector GUI, needs to called for the GUI elements to update
    //! in response to changes in the SolveCollection.
    void updateCollections();

    //! Show the specified solve collection and step. Provide -1 for the last step.
    void showSolveStep(int collection, int step);

  private:

    //! Initialize the Inspector GUI.
    void initInspector();

    //! Update the steps slider in response to changes in collection selection.
    void updateSteps();

    // Private variables
    Viewer *viewer;                   //!< Pointer to the main viewer.
    rt::SolveCollection solve_col;    //!< Record of steps taken to solve the problem.

    // Private GUI variables
    QGroupBox *g_collection=nullptr;  //!< Group box containing collection related elements.
    QGroupBox *g_step=nullptr;        //!< Group box containing step related elements.
    QSlider *s_collection=nullptr;    //!< Slider to specify which collection index to show.
    QSlider *s_step=nullptr;          //!< Slider to specify which step to show.
  };

}


#endif
