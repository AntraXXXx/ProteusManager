import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: mainMenuPage
    property StackView appStack
    property bool isLocalDatabase: appController.isLocalDatabase
    property bool databaseConnected: appController.databaseConnected

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        ScrollView {
            id: mainMenuScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: mainMenuScroll.availableWidth
                spacing: 18

                Label {
                    text: "Proteus Manager"
                    font.pixelSize: 40
                    font.bold: true
                    color: "#101828"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    text: "AI-assisted database schema and code generation"
                    font.pixelSize: 17
                    color: "#667085"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Frame {
                    Layout.fillWidth: true
                    padding: 22

                    background: Rectangle {
                        color: "white"
                        radius: 18
                        border.color: "#d0d5dd"
                    }

                    ColumnLayout {
                        spacing: 18
                        anchors.fill: parent

                        Label {
                            text: "Project Settings"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#101828"
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            columns: mainMenuPage.width < 760 ? 1 : 2
                            columnSpacing: 26
                            rowSpacing: 18
                            Layout.fillWidth: true

                            Label {
                                text: "Code Language:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: comboBox_CodeLanguage
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 300
                                    font.pixelSize: 16
                                    model: appController.codeLanguages()

                                    onCurrentIndexChanged: {
                                        appController.setSelectedLanguage(currentIndex)
                                    }
                                }

                                SettingHelpButton {
                                    objectName: "languageHelpButton"
                                    helpText: "Chooses the target language for generated application code, file names and database access patterns."
                                }
                            }

                            Label {
                                text: "AI Model:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: comboBox_AiModell
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 300
                                    font.pixelSize: 16
                                    enabled: appController.ollamaRunning && count > 0
                                    model: appController.availableModels

                                    onCurrentTextChanged: {
                                        if (currentText.length > 0) {
                                            appController.setSelectedModel(currentText)
                                        }
                                    }
                                }

                                SettingHelpButton {
                                    objectName: "aiModelHelpButton"
                                    helpText: "Selects the installed Ollama model used for generation. A suitable model improves structured, valid output."
                                }
                            }

                            Label {
                                text: "Ollama Endpoint:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                TextField {
                                    id: lineEdit_ollamaEndpoint
                                    Layout.fillWidth: true
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

                                SettingHelpButton {
                                    objectName: "ollamaEndpointHelpButton"
                                    helpText: "Sets the Ollama API address. Keep localhost for a private local model or use only a trusted secured endpoint."
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
                                Layout.fillWidth: true
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
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "Local Database:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Switch {
                                    checked: isLocalDatabase

                                    onCheckedChanged: {
                                        isLocalDatabase = checked
                                        appController.setIsLocalDatabase(checked)
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                SettingHelpButton {
                                    objectName: "databaseModeHelpButton"
                                    helpText: "Uses a database file on this computer. Turn it off to connect to a database server over the network."
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    padding: 22

                    background: Rectangle {
                        color: "white"
                        radius: 18
                        border.color: "#d0d5dd"
                    }

                    ColumnLayout {
                        spacing: 16
                        anchors.fill: parent

                        Label {
                            text: isLocalDatabase ? "Local Database" : "Online Database"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#101828"
                            Layout.fillWidth: true
                        }

                        GridLayout {
                        visible: isLocalDatabase
                        Layout.fillWidth: true
                            columns: mainMenuPage.width < 680 ? 1 : 2
                            columnSpacing: 12
                            rowSpacing: 12

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

                        Label {
                            text: "Database File:"
                            font.pixelSize: 18
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            TextField {
                                id: lineEdit_localdatabasepath
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                font.pixelSize: 16
                                leftPadding: 14
                                horizontalAlignment: TextInput.AlignLeft
                                verticalAlignment: TextInput.AlignVCenter
                                placeholderText: "Select local database file..."
                            }

                            Button {
                                text: "Browse"
                                font.pixelSize: 15
                                Layout.preferredWidth: 90
                                Layout.preferredHeight: 40

                                onClicked: fileDialog.open()
                            }

                            SettingHelpButton {
                                objectName: "localDatabasePathHelpButton"
                                helpText: "Chooses the local database file. The connection uses this file directly, so select a backup before risky operations."
                            }
                        }

                        Connections {
                            target: appController

                            function onDatabasePathChanged(path)
                            {
                                lineEdit_localdatabasepath.text = path
                            }
                        }
                    }

                        GridLayout {
                            visible: !isLocalDatabase
                            Layout.fillWidth: true
                            columns: mainMenuPage.width < 680 ? 1 : 2
                            columnSpacing: 20
                            rowSpacing: 12

                            Label {
                                text: "Database Type:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                ComboBox {
                                    id: comboBox_DatabaseDriver
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    model: appController.databaseDriverNames()
                                }

                                SettingHelpButton {
                                    objectName: "databaseDriverHelpButton"
                                    helpText: "Chooses the server driver and SQL dialect so connection and generated commands match the database system."
                                }
                            }

                            Label {
                                text: "Database Address:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: lineEdit_DataBaseAddress
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    placeholderText: "Database name"
                                }

                                SettingHelpButton {
                                    objectName: "databaseNameHelpButton"
                                    helpText: "Names the database on the server. This keeps the connection scoped to the intended data store."
                                }
                            }

                            Label {
                                text: "Host Name:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: lineEdit_HostName
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    placeholderText: "Host name"
                                }

                                SettingHelpButton {
                                    objectName: "hostNameHelpButton"
                                    helpText: "Sets the trusted server name or IP address. Verify it to avoid sending credentials to the wrong host."
                                }
                            }

                            Label {
                                text: "Port:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: lineEdit_Port
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    placeholderText: "Optional, for example 3306 or 5432"
                                    inputMethodHints: Qt.ImhDigitsOnly
                                }

                                SettingHelpButton {
                                    objectName: "portHelpButton"
                                    helpText: "Sets the server port. Leave it empty for the driver default or enter the port configured by the administrator."
                                }
                            }

                            Label {
                                text: "User Name:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: lineEdit_UserName
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    placeholderText: "User name"
                                }

                                SettingHelpButton {
                                    objectName: "userNameHelpButton"
                                    helpText: "Uses this database account. Prefer a dedicated account with only the permissions ProteusManager needs."
                                }
                            }

                            Label {
                                text: "Password:"
                                font.pixelSize: 18
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: lineEdit_Password
                                    Layout.fillWidth: true
                                    font.pixelSize: 16
                                    placeholderText: "Password"
                                    echoMode: TextInput.Password
                                }

                                SettingHelpButton {
                                    objectName: "passwordHelpButton"
                                    helpText: "Authenticates the database account. The value stays masked; use a unique secret and encrypted server connection."
                                }
                            }
                        }
            }
        }
        }
        }

        GridLayout {
            id: mainActionBar
            Layout.fillWidth: true
            columns: mainMenuPage.width < 560 ? 1 : mainMenuPage.width < 860 ? 2 : 5
            columnSpacing: 14
            rowSpacing: 10

            Button {
                id: pushButton_ConnectDB
                property string connectionState: "neutral"
                text: databaseConnected ? "Connected" : "Connect DB"

                enabled: !databaseConnected
                         && (isLocalDatabase
                             ? lineEdit_localdatabasepath.text.length > 0
                             : lineEdit_DataBaseAddress.text.length > 0
                               && lineEdit_HostName.text.length > 0
                               && comboBox_DatabaseDriver.currentText.length > 0)
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 130
                Layout.preferredHeight: 48

                onClicked: {
                    if (isLocalDatabase) {
                        appController.connectDatabase(
                            lineEdit_localdatabasepath.text
                        )
                    } else {
                        appController.connectOnlineDatabase(
                            comboBox_DatabaseDriver.currentText,
                            lineEdit_DataBaseAddress.text,
                            lineEdit_HostName.text,
                            lineEdit_Port.text,
                            lineEdit_UserName.text,
                            lineEdit_Password.text
                        )
                    }
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
                Layout.fillWidth: true
                Layout.minimumWidth: 145
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
                    Layout.fillWidth: true
                    Layout.minimumWidth: 150
                    Layout.preferredHeight: 48

                    onClicked: {
                        appStack.push(
                            Qt.resolvedUrl("ProgrammingCodeGeneratorPage.qml"),
                            { appStack: appStack }
                        )
                    }
                }

            Button {
                id: pushButton_Normalization
                text: "Normalize"
                enabled: databaseConnected && appController.aiEnvironmentReady
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 130
                Layout.preferredHeight: 48

                onClicked: {
                    appStack.push(
                        Qt.resolvedUrl("NormalizationPage.qml"),
                        { appStack: appStack }
                    )
                }
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
                Layout.fillWidth: true
                Layout.minimumWidth: 110
                Layout.preferredHeight: 48
                onClicked: Qt.quit()
            }
        }
    }
}
