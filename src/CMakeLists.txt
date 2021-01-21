cmake_minimum_required(VERSION 3.10)

project(pinrouter)

# import Qt5 dependencies
set(QT_VERSION_REQ "5.2")
find_package(Qt5Core ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Gui ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Widgets ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Test ${QT_VERSION_REQ} REQUIRED)

# general settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
enable_testing()

# add resources
qt5_add_resources(CUSTOM_RSC qrc/application.qrc)

# source and header files
set(BIN_SOURCES
    spatial.cc
    gui/mainwindow.cc
    gui/viewer.cc
    gui/settings.cc
    gui/route_inspector.cc
    gui/invoker.cc
    gui/prim/cell.cc
    router/router.cc
    router/route_graph.cc
    router/problem.cc
    router/routing_records.cc
    router/algs/alg.cc
    router/algs/a_star.cc
    router/algs/lee_moore.cc
    )
set(BIN_HEADERS
    spatial.h
    gui/mainwindow.h
    gui/viewer.h
    gui/settings.h
    gui/route_inspector.h
    gui/invoker.h
    gui/prim/cell.h
    router/router.h
    router/route_graph.h
    router/problem.h
    router/routing_records.h
    router/algs/alg.h
    router/algs/a_star.h
    router/algs/lee_moore.h
    )

# libraries to be linked
set(LIB_LINKS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    )

# inclusions
include_directories(.)

# build application
add_executable(pinrouter main.cc ${BIN_SOURCES} ${BIN_HEADERS} ${CUSTOM_RSC})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_LINKS})

# build unit tests
add_executable(pinrouter_tests tests/pinrouter_tests.cpp ${BIN_SOURCES} ${BIN_HEADERS} ${CUSTOM_RSC})
target_link_libraries(pinrouter_tests Qt5::Test ${LIB_LINKS})
add_test(pinrouter_tests pinrouter_tests)
set_tests_properties(pinrouter_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
add_custom_command(TARGET pinrouter_tests
    POST_BUILD
    COMMAND ctest -C $<CONFIGURATION> --output-on-failure)
