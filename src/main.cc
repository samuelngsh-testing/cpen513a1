// @file:     main.cc
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     Main function for the routing algorithm implementation. The GUI is
//            implemented using the Qt5 framework.

#include <QApplication>
#include <QCommandLineParser>
#include <QMainWindow>
#include <QDebug>

#include "gui/mainwindow.h"

int main(int argc, char **argv) {
  // initialize QApplication
  QApplication app(argc, argv);
  app.setApplicationName("Pin Routing Application");

  // specify possible command line inputs
  QCommandLineParser parser;
  parser.setApplicationDescription("Pin Routing Application for CPEN 513 by "
      "Samuel Ng.");
  parser.addHelpOption();
  parser.addPositionalArgument("in_file", "Input file specifying the problem to"
      " be routed (optional, can be selected from the GUI).");
  QCommandLineOption cachePathOption("cache_path", "Specify a directory path "
      "for cache to be written to. Defaults to somewhere in the system tmp "
      "directories if unspecified.", "path");
  parser.addOption(cachePathOption);
  parser.process(app);

  // get input file path
  const QStringList args = parser.positionalArguments();
  QString in_path;
  if (!args.empty()) {
    in_path = args[0];
    qDebug() << QObject::tr("Input file path: %1").arg(in_path);
  }

  // show the main GUI
  gui::MainWindow mw(in_path);
  mw.show();

  // run app
  return app.exec();
}
