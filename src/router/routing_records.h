// @file:     router_records.h
// @author:   Samuel Ng
// @created:  2021-01-20
// @license:  GNU LGPL v3
//
// @desc:     Classes and structs relevant to keeping routing records.

#ifndef _RT_ROUTING_RECORDS_H_
#define _RT_ROUTING_RECORDS_H_

#include <QObject>
#include "spatial.h"

namespace rt {

  //! Store information related to a solve attempt within a collection.
  struct SolveSteps
  {
    ~SolveSteps() {step_grids.clear();}

    //! Convenient index operator to access a step grid.
    sp::Grid *operator[](int i) {return step_grids.value(i);}

    QString desc;                 //!< Description of this attempt.
    QList<sp::Grid*> step_grids;  //!< A list of grids containing every step in the solve attempt
  };

  //! Store information on a collection of solve attempts.
  struct SolveCollection
  {

    //! Clear the collection
    void clear() {desc = ""; solve_steps.clear();}

    //! Convenient index operator to access a particular collection of SolveSteps
    SolveSteps operator[](int i) {return solve_steps.value(i);}

    //! Add a new SolveSteps object and return its pointer
    SolveSteps *newSolveSteps()
    {
      solve_steps.append(SolveSteps());
      return &solve_steps.back();
    }
    
    QString desc;                   //!< Description of this collection.
    QList<SolveSteps> solve_steps;  //!< A list of solve steps.
  };

  //! Solve step storage detail level
  enum LogVerbosity{LogAllIntermediate, LogCoarseIntermediate, LogResultsOnly};

  //! Real time update verbosity
  enum GuiUpdateVerbosity{VisualizeAllIntermediate, VisualizeCoarseIntermediate,
    VisualizeResultsOnly};

  //! A class that fascilitates the recording keeping of routing steps.
  class RoutingRecords : public QObject
  {
    Q_OBJECT
  public:

    //! Constructor taking the current verbosity level settings as well as the
    //! SolveCollection to write to.
    RoutingRecords(LogVerbosity log_vb=LogCoarseIntermediate,
        GuiUpdateVerbosity gui_vb=VisualizeCoarseIntermediate,
        SolveCollection *solve_col=nullptr)
      : log_verbosity(log_vb), gui_verbosity(gui_vb), solve_col(solve_col) {};

    //! Destructor.
    ~RoutingRecords() {};

    //! Set the log verbosity.
    void setLogVerbosity(LogVerbosity log_vb) {log_verbosity = log_vb;}

    //! Return the log verbosity.
    LogVerbosity logVerbosity() const {return log_verbosity;}

    //! Set the GUI verbosity.
    void setGuiVerbosity(GuiUpdateVerbosity gui_vb) {gui_verbosity = gui_vb;}

    //! Return the GUI verbosity.
    GuiUpdateVerbosity guiVerbosity() const {return gui_verbosity;}

    //! Set a new solve collection.
    void setSolveCollection(SolveCollection *scol) {solve_col = scol;}

    //! Return the current solve collection.
    SolveCollection *setSolveCollection() {return solve_col;}

    //! Create a new set of solve steps in the collection.
    SolveSteps *newSolveSteps();

    //! Log the provided cell grid to the latest solve step in the collection. 
    //! The caller must also indicate the intended verbosity level of the event.
    //! If the indicated verbosity is higher than the internal settings, the
    //! event won't be logged.
    void logCellGrid(sp::Grid *cell_grid, LogVerbosity log_vb,
        GuiUpdateVerbosity gui_vb);

  signals:

    //! Emit a signal containing the newly logged router step reults. Intended 
    //! to be sent to the GUI side for real time updates, if needed.
    void routerStep(sp::Grid *grid);

  private:

    LogVerbosity log_verbosity;             //!< The verbosity of logged steps.
    GuiUpdateVerbosity gui_verbosity;       //!< The verbosity of steps shown in real time.
    SolveCollection *solve_col=nullptr;     //!< SolveCollection to log to.
    SolveSteps *curr_solve_steps=nullptr;   //!< Current solve steps (from solve_col)
  };

}


#endif
