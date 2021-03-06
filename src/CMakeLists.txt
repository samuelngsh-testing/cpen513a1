set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")
cmake_minimum_required(VERSION 3.10)

project(pinrouter)

# import Qt5 dependencies
set(QT_VERSION_REQ "5.2")
find_package(Qt5Core ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Gui ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Widgets ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Svg ${QT_VERSION_REQ} REQUIRED)
find_package(Qt5Test ${QT_VERSION_REQ} REQUIRED)

# general settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
enable_testing()

# add resources
qt5_add_resources(CUSTOM_RSC qrc/application.qrc)

# source and header files
set(LIB_SOURCES
    spatial.cc
    gui/mainwindow.cc
    gui/viewer.cc
    gui/settings.cc
    gui/route_inspector.cc
    gui/invoker.cc
    gui/prim/cell.cc
    router/router.cc
    router/problem.cc
    router/routing_records.cc
    router/algs/alg.cc
    router/algs/a_star.cc
    router/algs/lee_moore.cc
    )
set(LIB_HEADERS
    spatial.h
    gui/mainwindow.h
    gui/viewer.h
    gui/settings.h
    gui/route_inspector.h
    gui/invoker.h
    gui/prim/cell.h
    router/router.h
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
    Qt5::Svg
    )

# inclusions
include_directories(.)

# build application library (shared by application and unit tests)
# NOTE the library method only works on CMake 3.12 or above. Reverting to building
#      the program and unit tests twice for compatibility reasons
#add_library(pinrouter_lib OBJECT ${LIB_SOURCES} ${LIB_HEADERS} ${CUSTOM_RSC})
#target_link_libraries(pinrouter_lib PUBLIC ${LIB_LINKS})

# build application
add_executable(pinrouter MACOSX_BUNDLE main.cc ${LIB_SOURCES} ${LIB_HEADERS} ${CUSTOM_RSC})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_LINKS})
set_target_properties(pinrouter PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.pinrouter
    MACOSX_BUNDLE_BUNDLE_NAME pinrouter
    MACOSX_BUNDLE_BUNDLE_VERSION "0.0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.0.1"
)
#add_executable(pinrouter main.cc)
#target_link_libraries(${PROJECT_NAME} PUBLIC pinrouter_lib)

# build unit tests
add_executable(pinrouter_tests tests/pinrouter_tests.cpp ${LIB_SOURCES} ${LIB_HEADERS} ${CUSTOM_RSC})
target_link_libraries(pinrouter_tests Qt5::Test ${LIB_LINKS})
#add_executable(pinrouter_tests tests/pinrouter_tests.cpp)
#target_link_libraries(pinrouter_tests pinrouter_lib Qt5::Test)
add_test(pinrouter_tests pinrouter_tests)
set_tests_properties(pinrouter_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
add_custom_command(TARGET pinrouter_tests
    POST_BUILD
    COMMAND ctest -C $<CONFIGURATION> --output-on-failure)

# install the binary
install(TARGETS pinrouter
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
