// @file:     problem_tests.cpp
// @author:   Samuel Ng
// @created:  2021-01-14
// @license:  GNU LGPL v3
//
// @desc:     Unit tests that ensure the Problem class reads a known problem 
//            file properly into the needed lists and maps.

#include <QtTest/QtTest>
#include "router/problem.h"

class RouterTests : public QObject
{
  Q_OBJECT

// functions in these slots are automatically called after compilation
private slots:

  void testProblemRead()
  {
    using namespace rt;
    Problem problem(":/sample_problems/sydney.infile");

    
    QCOMPARE(problem.isValid(), true);  // the problem is expected to be valid
    // TODO compare dimensions against expectation
    // TODO check all contents of cellMap
    // TODO check all contents of pinSetIdMap
  }

  // TODO test that color generator doesn't crash with the inclusion of more colors (than the default thresholds)

};

QTEST_MAIN(RouterTests)
#include "pinrouter_tests.moc"  // generated at compile time
