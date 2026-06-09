import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    property StackView appStack
    property bool isLocalDatabase: appController.isLocalDatabase
    property bool databaseConnected: appController.databaseConnected

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 34
        spacing: 24

        Label {
            text: "Proteus Manager"
            font.pixelSize: 40
            font.bold: true
            color: "#101828"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "AI-assisted database schema and code generation"
            font.pixelSize: 17
            color: "#667085"
            Layout.alignment: Qt.AlignHCenter
        }

        Frame {
            Layout.fillWidth: true
            padding: 26

            background: Rectangle {
                color: "white"
                radius: 18
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                spacing: 18

                Label {
                    text: "Project Settings"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#101828"
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 26
                    rowSpacing: 18
                    Layout.fillWidth: true

                    Label {
                        text: "Code Language:"
                        font.pixelSize: 18
                    }

                    ComboBox {
                        id: comboBox_CodeLanguage
                        Layout.fillHeight: true
                        font.pixelSize: 16
                        Layout.preferredWidth: 300
                        model: appController.codeLanguages()

                        onCurrentIndexChanged: {
                            appController.setSelectedLanguage(currentIndex)
                        }
                    }

                    Label {
                        text: "AI Model:"
                        font.pixelSize: 18
                    }

                    ComboBox {
                        id: comboBox_AiModell
                        Layout.fillHeight: true
                        font.pixelSize: 16
                        Layout.preferredWidth: 300
                        enabled: appController.ollamaRunning && count > 0
                        model: appController.availableModels

                        onCurrentTextChanged: {
                            if (currentText.length > 0) {
                                appController.setSelectedModel(currentText)
                            }
                        }
                    }

                    Label {
                        text: "Ollama Endpoint:"
                        font.pixelSize: 18
                    }

                    RowLayout {
                        Layout.preferredWidth: 520
                        spacing: 10

                        TextField {
                            id: lineEdit_ollamaEndpoint
                            Layout.preferredWidth: 330
                            Layout.preferredHeight: 35
                            font.pixelSize: 16
                            text: appController.ollamaEndpoint
                            placeholderText: "http://localhost:11434"

                            onEditingFinished: {
                                appController.setOllamaEndpoint(text)
                            }
                        }

                        Button {
                            text: "Check"
                            font.pixelSize: 16
                            Layout.preferredWidth: 90
                            Layout.preferredHeight: 38

                            onClicked: {
                                if (lineEdit_ollamaEndpoint.text !== appController.ollamaEndpoint) {
                                    appController.setOllamaEndpoint(lineEdit_ollamaEndpoint.text)
                                } else {
                                    appController.refreshAiEnvironment()
                                }
                            }
                        }
                    }

                    Label {
                        text: "AI Status:"
                        font.pixelSize: 18
                    }

                    Label {
                        text: appController.aiConnectionStatus
                        font.pixelSize: 16
                        color: appController.aiEnvironmentReady ? "#15803d" : "#b45309"
                        wrapMode: Text.WordWrap
                        Layout.preferredWidth: 520
                    }

                    Label {
                        text: "Setup:"
                        font.pixelSize: 18
                    }

                    Label {
                        text: appController.aiSetupInstructions
                        font.pixelSize: 15
                        color: "#475467"
                        wrapMode: Text.WordWrap
                        Layout.preferredWidth: 520
                    }

                    Label {
                        text: "Local Database:"
                        font.pixelSize: 18
                    }

                    Switch {
                        checked: isLocalDatabase

                        onCheckedChanged: {
                            isLocalDatabase = checked
                            appController.setIsLocalDatabase(checked)
                        }
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            padding: 26

            background: Rectangle {
                color: "white"
                radius: 18
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                spacing: 16

                Label {
                    text: isLocalDatabase ? "Local Database" : "Online Database"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#101828"
                }

                RowLayout {
                        visible: isLocalDatabase
                        Layout.fillWidth: true
                        spacing: 12

                        FileDialog {
                            id: fileDialog
                            title: "Select Database"
                            onAccepted: {
                                let path = fileDialog.selectedFile.toString();
                                path = path.replace(/^file:\/\/\//, "");
                                if (path.match(/^\/[a-zA-Z]:\//)) {
                                    path = path.substring(1);
                                }
                                lineEdit_localdatabasepath.text = path;
                            }
                        }

                        Button {
                            text: "Add"
                            font.pixelSize: 16
                            Layout.preferredWidth: 90
                            Layout.preferredHeight: 42

                            onClicked: {
                                fileDialog.open()
                            }
                        }

                        TextField {
                            id: lineEdit_localdatabasepath

                            Layout.preferredWidth: 750
                            Layout.preferredHeight: 35
                            Layout.alignment: Qt.AlignHCenter

                            font.pixelSize: 16
                            leftPadding: 18

                            horizontalAlignment: TextInput.AlignLeft
                            verticalAlignment: TextInput.AlignVCenter

                            placeholderText: "Select local database file..."
                        }

                        Connections {
                            target: appController

                            function onDatabasePathChanged(path)
                            {
                                lineEdit_localdatabasepath.text = path
                            }
                        }
                    }

                ColumnLayout {
                    visible: !isLocalDatabase
                    Layout.fillWidth: true
                    spacing: 12

                    TextField {
                        id: lineEdit_DataBaseAddress
                        Layout.fillWidth: true
                        font.pixelSize: 16
                        placeholderText: "Database address"
                    }

                    TextField {
                        id: lineEdit_HostName
                        Layout.fillWidth: true
                        font.pixelSize: 16
                        placeholderText: "Host name"
                    }

                    TextField {
                        id: lineEdit_Password
                        Layout.fillWidth: true
                        font.pixelSize: 16
                        placeholderText: "Password"
                        echoMode: TextInput.Password
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                id: pushButton_ConnectDB
                property string connectionState: "neutral"
                text: databaseConnected ? "Connected" : "Connect DB"

                enabled: isLocalDatabase ? lineEdit_localdatabasepath.text.length > 0 && !databaseConnected : true
                font.pixelSize: 16
                Layout.preferredWidth: 145
                Layout.preferredHeight: 48

                onClicked: {
                    appController.connectDatabase(
                    lineEdit_localdatabasepath.text
                    )
                }

                background: Rectangle {
                    radius: 8
                    color: databaseConnected ? "#22c55e" : pushButton_ConnectDB.connectionState === "failed" ? "#ef4444" : "#e5e7eb"
                    border.color: "#d0d5dd"
                }
            }

            Button {
                id: pushButton_SqlGenerator
                text: "SQL Generator"
                enabled: databaseConnected && appController.aiEnvironmentReady
                font.pixelSize: 16
                Layout.preferredWidth: 165
                Layout.preferredHeight: 48

                onClicked: {
                    appStack.push(
                    Qt.resolvedUrl("SqlGeneratorPage.qml"),
                    { appStack: appStack }
                    )
                }
            }

            Button {
                    id: pushButton_CodeGenerator

                    text: "Code Generator"
                    enabled: databaseConnected && appController.aiEnvironmentReady

                    font.pixelSize: 16
                    Layout.preferredWidth: 175
                    Layout.preferredHeight: 48

                    onClicked: {
                        appStack.push(
                            Qt.resolvedUrl("ProgrammingCodeGeneratorPage.qml"),
                            { appStack: appStack }
                        )
                    }
                }
            Item {
                Layout.fillWidth: true
            }

            Connections {
                    target: appController

                    function onDatabaseConnectedChanged(connected) {
                        databaseConnected = connected

                        if (connected) {
                            pushButton_ConnectDB.connectionState = "connected"
                        } else {
                            pushButton_ConnectDB.connectionState = "failed"
                        }
                    }

                    function onIsLocalDatabaseChanged() {
                        isLocalDatabase = appController.isLocalDatabase
                    }
                }
            Button {
                text: "Exit"
                font.pixelSize: 16
                Layout.preferredWidth: 125
                Layout.preferredHeight: 48
                onClicked: Qt.quit()
            }
        }
    }
}
