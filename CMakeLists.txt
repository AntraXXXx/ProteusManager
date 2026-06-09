cmake_minimum_required(VERSION 3.19)
project(DataBaseManager LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    Quick
    Qml
    QuickControls2
    Sql
    Network
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
    utils/dalfileexporter.cpp
    utils/dalfileexporter.h
    utils/sqloutputrecorder.cpp
    utils/sqloutputrecorder.h

    utils/classscanner.h
    utils/classscanner.cpp

    utils/parsedclass.h
    utils/parsedattribute.h

    utils/classparser.h
    utils/classparser.cpp

    client/ollamaclient.h
    client/ollamaclient.cpp

    database/databasemanager.h
    database/databasemanager.cpp

    utils/programminglanguage.h
    utils/programminglanguage.cpp
)

qt_add_qml_module(DataBaseManager
    URI DataBaseManager
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/MainMenuPage.qml
        qml/SqlGeneratorPage.qml
        qml/ProgrammingCodeGeneratorPage.qml
        SOURCES appcontroller.h appcontroller.cpp
)

target_link_libraries(DataBaseManager PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
)