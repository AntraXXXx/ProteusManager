cmake_minimum_required(VERSION 3.19)
project(DataBaseManager LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(DataBaseManager
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui



    databasecontroller.ui
    textfilemanager.h
    textfilemanager.cpp
    databasecontroller.h databasecontroller.cpp
    tablegenerator.ui
    tablegenerator.h tablegenerator.cpp



)

target_link_libraries(DataBaseManager
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS DataBaseManager
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DataBaseManager
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
