cmake_minimum_required(VERSION 3.19)
project(DataBaseManager LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Network
    Sql
)

qt_standard_project_setup()

qt_add_executable(DataBaseManager
    WIN32 MACOSX_BUNDLE
    main.cpp

    windows/mainwindow.ui
    windows/tablegenerator.ui
    windows/databasecontroller.ui

    windows/mainwindow.h
    windows/tablegenerator.h
    windows/databasecontroller.h

    windows/mainwindow.cpp
    windows/tablegenerator.cpp
    windows/databasecontroller.cpp

    utils/textfilemanager.cpp

    utils/textfilemanager.h
    utils/classscanner.h utils/classscanner.cpp

    utils/parsedclass.h
    utils/parsedattribute.h
    utils/classparser.h utils/classparser.cpp
    client/ollamaclient.h client/ollamaclient.cpp
    database/databasemanager.h
    database/databasemanager.cpp



)

target_link_libraries(DataBaseManager
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Sql
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
