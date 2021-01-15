// @file:     viewer.h
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     The Viewer class provides the portal for viewing the problem and 
//            the solving steps.

#ifndef _GUI_VIEWER_H_
#define _GUI_VIEWER_H_

#include <QtWidgets>
#include "settings.h"
#include "router/problem.h"
#include "router/router.h"
#include "prim/cell.h"

namespace gui {

  class Viewer : public QGraphicsView
  {
    Q_OBJECT

  public:

    //! Constructor.
    Viewer(QWidget *parent=nullptr);

    //! Destructor.
    ~Viewer();

    //! Instruct viewer to show the provided problem.
    void showProblem(const rt::Problem &problem);

    //! Instruct viewer to clear any existing problem.
    void clearProblem();

    //! Fit problem in viewport.
    void fitProblemInView();

    //! Refresh the viewer with the provided cell grid
    void updateCellGrid(sp::Grid *cell_grid=nullptr);

  private:

    //! Initialize the viewer's GUI elements.
    void initViewer();

    // Private variables
    QGraphicsScene *scene=nullptr;  //!< Pointer to the scene object.
    rt::Problem curr_problem;       //!< Current problem being shown.
    QList<Cell*> cells;             //!< List of cell pointers currently shown.
  };

}


#endif
