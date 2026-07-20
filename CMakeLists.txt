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
    Test
)

qt_standard_project_setup()
include(CTest)

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
    utils/codegenerationprofile.cpp
    utils/codegenerationprofile.h
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
    client/ollamaenvironment.h
    client/ollamaenvironment.cpp

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
        qml/NormalizationPage.qml
        qml/SchemaDiagramView.qml
        qml/SchemaTableNode.qml
        qml/SettingHelpButton.qml
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

if(BUILD_TESTING)
    set(PROTEUS_TEST_TARGETS)

    function(add_proteus_test target_name)
        add_executable(${target_name} ${ARGN})
        target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_compile_definitions(${target_name} PRIVATE
            PROTEUS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )
        target_link_libraries(${target_name} PRIVATE
            Qt6::Core
            Qt6::Test
        )
        add_test(NAME ${target_name} COMMAND ${target_name})

        if(WIN32)
            get_filename_component(PROTEUS_COMPILER_BIN_DIR
                "${CMAKE_CXX_COMPILER}"
                DIRECTORY
            )

            if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22")
                set(PROTEUS_TEST_PATH_MODIFICATIONS
                    "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
                    "PATH=path_list_prepend:${PROTEUS_COMPILER_BIN_DIR}"
                )
                set_tests_properties(${target_name} PROPERTIES
                    ENVIRONMENT_MODIFICATION
                        "${PROTEUS_TEST_PATH_MODIFICATIONS}"
                )
            else()
                set_tests_properties(${target_name} PROPERTIES
                    ENVIRONMENT
                        "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;${PROTEUS_COMPILER_BIN_DIR}\\;$ENV{PATH}"
                )
            endif()
        endif()

        set(PROTEUS_TEST_TARGETS
            ${PROTEUS_TEST_TARGETS}
            ${target_name}
            PARENT_SCOPE
        )
    endfunction()

    add_proteus_test(ClassParserTest
        tests/classparsertest.cpp
        utils/classparser.cpp
        utils/programminglanguage.cpp
    )

    add_proteus_test(ClassScannerTest
        tests/classscannertest.cpp
        utils/classscanner.cpp
        utils/programminglanguage.cpp
    )

    add_proteus_test(DatabaseManagerTest
        tests/databasemanagertest.cpp
        database/databasemanager.cpp
    )
    target_link_libraries(DatabaseManagerTest PRIVATE
        Qt6::Sql
    )

    add_proteus_test(SqlValidationTest
        tests/sqlvalidationtest.cpp
        database/databasemanager.cpp
    )
    target_link_libraries(SqlValidationTest PRIVATE
        Qt6::Sql
    )

    add_proteus_test(DalGenerationTest
        tests/dalgenerationtest.cpp
        utils/dalfileexporter.cpp
        utils/codegenerationprofile.cpp
        utils/programminglanguage.cpp
    )

    add_proteus_test(SqlOutputRecorderTest
        tests/sqloutputrecordertest.cpp
        utils/sqloutputrecorder.cpp
    )

    add_proteus_test(OllamaClientTest
        tests/ollamaclienttest.cpp
        client/ollamaclient.cpp
    )
    target_link_libraries(OllamaClientTest PRIVATE
        Qt6::Network
    )

    add_proteus_test(OllamaEnvironmentTest
        tests/ollamaenvironmenttest.cpp
        client/ollamaenvironment.cpp
    )

    add_proteus_test(QmlWorkflowTest
        tests/qmlworkflowtest.cpp
    )
    target_link_libraries(QmlWorkflowTest PRIVATE
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
    )

    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${PROTEUS_TEST_TARGETS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
