// @file:     mainwindow.cc
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the MainWindow class.

#include "mainwindow.h"
#include "router/router.h"

using namespace gui;

MainWindow::MainWindow(const QString &in_path, QWidget *parent)
  : QMainWindow(parent), problem(in_path)
{
  initGui();
  if (problem.isValid()) {
    viewer->showProblem(problem);
  }
  open_dir_path = (in_path.isEmpty()) ? QDir::currentPath() : QFileInfo(in_path).absolutePath();
}

void MainWindow::aboutDialog()
{
  QString app_name = QCoreApplication::applicationName();

  QMessageBox::about(this, tr("About"), tr("Application: %1\nAuthor: Samuel Ng")
      .arg(app_name));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
  viewer->fitProblemInView();

  // also call the original method
  QMainWindow::resizeEvent(e);
}

void MainWindow::readAndShowProblem(const QString &in_path)
{
  problem = rt::Problem(in_path);
  viewer->showProblem(problem);
  open_dir_path = QFileInfo(in_path).absolutePath();
}

void MainWindow::initGui()
{
  initMenuBar();

  viewer = new gui::Viewer();
  inspector = new gui::RouteInspector(viewer);

  QVBoxLayout *vbl = new QVBoxLayout(); // main vertical layout
  vbl->addWidget(viewer);
  vbl->addWidget(inspector);

  QWidget *w_main = new QWidget(this);  // main widget that holds everything
  w_main->setLayout(vbl);
  setCentralWidget(w_main);
}

void MainWindow::initMenuBar()
{
  // initialize menus
  QMenu *file = menuBar()->addMenu(tr("&File"));
  QMenu *route = menuBar()->addMenu(tr("&Route"));
  QMenu *help = menuBar()->addMenu(tr("&Help"));

  // file menu actions
  QAction *open_problem = new QAction(tr("&Open..."), this);
  QAction *quit = new QAction(tr("&Quit"), this);
  QMenu *open_sample_problem = new QMenu(tr("Open Sample Problem"), this);

  // populate sample problems list
  QDir sp_dir(":/sample_problems");
  QFileInfoList sp_l = sp_dir.entryInfoList();
  for (QFileInfo sp : sp_l) {
    if (!sp.isFile())
      continue;
    QAction *open_sp = new QAction(sp.fileName(), this);
    open_sample_problem->addAction(open_sp);
    connect(open_sp, &QAction::triggered,
        [this, sp](){
          readAndShowProblem(sp.absoluteFilePath());
        });
  }

  // route menu actions
  QAction *lee_moore = new QAction(tr("Lee-Moore"), this);

  // help menu actions
  QAction *about = new QAction(tr("&About"));

  // assign keyboard shortcuts
  open_problem->setShortcut(tr("CTRL+O"));
  quit->setShortcut(tr("CTRL+Q"));

  // connect action signals
  connect(open_problem, &QAction::triggered,
      [this](){
        // open file dialog and load the specified file path
        QFileDialog fd;
        fd.setDefaultSuffix("infile");
        QString open_path = fd.getOpenFileName(this, tr("Open File"),
              open_dir_path, tr("In Files (*.infile);;All files (*.*)"));
        if (!open_path.isNull()) {
          readAndShowProblem(open_path);
        }
      });
  connect(quit, &QAction::triggered, this, &QWidget::close);
  connect(lee_moore, &QAction::triggered,
      [this]()
      {
        // TODO make generic, current implementation is hard-coded test
        inspector->clearCollections();
        if (problem.isValid()) {
          rt::Problem problem_cp(problem);
          rt::Router router(problem_cp, "/tmp/router");
          router.routeForId(rt::Router::LeeMoore, problem_cp.pinSets()[0], 
              problem_cp.cellGrid(), inspector->solveCollection());
          inspector->updateCollections();
        }
      });
  connect(about, &QAction::triggered, this, &MainWindow::aboutDialog);

  // add actions to the appropriate menus
  file->addAction(open_problem);
  file->addMenu(open_sample_problem);
  file->addAction(quit);
  route->addAction(lee_moore);
  help->addAction(about);
}
