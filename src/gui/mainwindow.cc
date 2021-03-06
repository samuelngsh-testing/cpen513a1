// @file:     mainwindow.cc
// @author:   Samuel Ng
// @created:  2021-01-13
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the MainWindow class.

#include <QSvgGenerator>
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
  setWindowTitle(tr("%1 - %2").arg(QCoreApplication::applicationName())
      .arg(QFileInfo(in_path).fileName()));
  problem = rt::Problem(in_path);
  inspector->clearCollections();
  viewer->showProblem(problem);
  invoker->setProblem(problem);
  open_dir_path = QFileInfo(in_path).absolutePath();
}

void MainWindow::takeScreenshot() const
{
  QFileDialog fd;
  QString svg_path = fd.getSaveFileName(this->parentWidget(), tr("Save svg to..."),
      "screenshot.svg");
  if (svg_path.isEmpty()) {
    return;
  }
  QRectF target = viewer->sceneRect();

  QSvgGenerator gen;
  gen.setFileName(svg_path);
  gen.setViewBox(target);

  QPainter painter;
  painter.begin(&gen);
  viewer->render(&painter, target);
  painter.end();
}

void MainWindow::initGui()
{
  initMenuBar();

  viewer = new gui::Viewer();
  inspector = new gui::RouteInspector(viewer);
  invoker = new gui::Invoker(viewer, inspector);

  QHBoxLayout *hbl = new QHBoxLayout(); // horizontal bottom layout
  hbl->addWidget(invoker);
  hbl->addWidget(inspector);

  QVBoxLayout *vbl = new QVBoxLayout(); // main vertical layout
  vbl->addWidget(viewer);
  vbl->addLayout(hbl);

  QWidget *w_main = new QWidget(this);  // main widget that holds everything
  w_main->setLayout(vbl);
  setCentralWidget(w_main);
}

void MainWindow::initMenuBar()
{
  // initialize menus
  QMenu *file = menuBar()->addMenu(tr("&File"));
  QMenu *tools = menuBar()->addMenu(tr("&Tools"));
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

  // tools menu actions
  QAction *screenshot = new QAction(tr("&Screenshot"));

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
  connect(screenshot, &QAction::triggered, this, &MainWindow::takeScreenshot);
  connect(about, &QAction::triggered, this, &MainWindow::aboutDialog);

  // add actions to the appropriate menus
  file->addAction(open_problem);
  file->addMenu(open_sample_problem);
  file->addSeparator();
  file->addAction(quit);
  tools->addAction(screenshot);
  help->addAction(about);
}
