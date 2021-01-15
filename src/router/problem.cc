// @file:     problem.cc
// @author:   Samuel Ng
// @created:  2021-01-12
// @license:  GNU LGPL v3
//
// @desc:     Implementation of the Problem class.

#include <QFile>
#include <QDir>
#include <QDebug>
#include "problem.h"

using namespace rt;

// Problem class implementations

// Constructor
Problem::Problem(const QString &in_path)
{
  // read problem file if specified, otherwise keep the problem as an empty one
  if (!in_path.isEmpty()) {
    bool success = readProblem(in_path);
    if (!success) {
      qFatal("Unable to read the specified input file.");
    }
  }
}

// Read problem
bool Problem::readProblem(const QString &in_path)
{
  // attempt to open the input file for reading
  QFile in_file(in_path);
  qDebug() << QObject::tr("Attempting to read input file %1...").arg(in_path);
  if (!in_file.open(QFile::ReadOnly | QFile::Text)) {
    qDebug() << "Unable to open file for reading.";
    return false;
  }

  // lambda function for splitting pairs of ints from strings and assigning 
  // them to provided refs; the delimitor can be changed
  auto readStrPairToInts = [](const QString &line, int &a, int &b, 
      const QString &delim=" ") -> bool
  {
    QStringList dims(line.split(delim));
    if (dims.size() == 2) {
      a = dims[0].toInt();
      b = dims[1].toInt();
      return true;
    }
    return false;
  };

  // lambda function for reading sinks into the provided coordinates ref.
  auto readGridPoints = [](const QString &line, sp::PinSet &coords,
      const QString &delim=" ") -> bool
  {
    QStringList pts(line.split(delim));
    int num_grid_pts = pts.takeFirst().toInt();
    while (!pts.empty()) {
      int x = pts.takeFirst().toInt();
      int y = pts.takeFirst().toInt();
      coords.append(sp::Coord(x, y));
    }
    if (num_grid_pts != coords.size()) {
      qDebug() << QObject::tr("%1 grid points expected but %2 points retrieved."
          " Something is wrong.").arg(num_grid_pts).arg(coords.size());
      return false;
    }
    return true;
  };

  // parse the file
  QTextStream out(&in_file);
  int remaining_o_cells=-1;
  int remaining_sinks=-1;
  ReadPhase phase = GridSize;
  while (!in_file.atEnd()) {
    QString line = in_file.readLine().trimmed();
    switch (phase)
    {
      // read grid size
      case GridSize:
        {
          bool success = readStrPairToInts(line, dim_x, dim_y);
          if (!success) {
            qDebug() << "Unexpected number of values received for GridSize";
            return false;
          }
          phase = ObsCellCount;   // proceed to next phase
          break;
        }
      // read obstruction cell count
      case ObsCellCount:
        {
          remaining_o_cells = line.toInt();
          phase = ObsCells;       // proceed to next phase
          break;
        }
      // read obstruction cells
      case ObsCells:
        {
          // read obstruction cell and add to list
          int x, y;
          bool success = readStrPairToInts(line, x, y);
          if (!success) {
            qDebug() << "Unexpected number of values received for obstruction "
                "cell coordinates";
          }
          obs_cells.append(sp::Coord(x, y));
          
          // bookkeeping
          if (--remaining_o_cells == 0) {
            phase = PinCount;  // proceed to next phase
          }
          break;
        }
      case PinCount:
        {
          remaining_sinks = line.toInt();
          phase = Pins;        // proceed to next phase
          break;
        }
      case Pins:
        {
          sp::PinSet grid_pts;
          bool success = readGridPoints(line, grid_pts);
          if (success) {
            pin_sets.append(grid_pts);
          } else {
            return false;
          }

          // bookkeeping
          if (--remaining_sinks == 0) {
            phase = Finished;
          }
          break;
        }
      case Finished:
      default:
        break;
    }
  }
  in_file.close();
  refreshGrid();
  qDebug() << "Successfully read the problem file.";
  return true;
}

bool Problem::isValid() const
{
  // simplest checks
  if (dim_x <= 0 || dim_y <= 0 || pin_sets.empty()) {
    return false;
  }

  // out of bound errors
  auto allInBound = [this](QList<sp::Coord> coords)->bool
  {
    // return whether all of the coordinates in a provided iterable containing
    // coords are within bounds
    return std::all_of(coords.begin(), coords.end(),
        [this](sp::Coord coord){return coord.isWithinBounds(dim_x, dim_y);});
  };
  // apply the boundary check on obstruction cells and sets of pins
  bool in_bound = std::all_of(pin_sets.begin(), pin_sets.end(), 
      [allInBound](sp::PinSet pin_set){return allInBound(pin_set);});
  in_bound &= allInBound(obs_cells);
  if (!in_bound) {
    qDebug() << "Problem contains out of bound pins or obstruction cells.";
    return false;
  }

  // find coordinate clashes between obstruction cells and pins
  for (sp::Coord obs_coord : obs_cells) {
    for (sp::PinSet pin_set : pin_sets) {
      for (sp::Coord pin_coord : pin_set) {
        if (obs_coord == pin_coord) {
          qDebug() << "Found clashing pin and obstruction cell coordinates.";
          return false;
        }
      }
    }
  }

  return true;
}

void Problem::refreshGrid()
{
  cell_grid = sp::Grid(dim_x, dim_y, obs_cells, pin_sets);
}

