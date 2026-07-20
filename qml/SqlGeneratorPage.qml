import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: sqlGeneratorPage
    property StackView appStack
    title: "SQL Generator"
    readonly property bool compactLayout: width < 700

    background: Rectangle {
        color: "#eef2f7"
    }

    Item {
        anchors.fill: parent
        anchors.margins: compactLayout ? 12 : 24

        Label {
            id: sqlPageTitle
            text: "SQL Generator"
            font.pixelSize: 32
            font.bold: true
            color: "#101828"
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        ScrollView {
            id: sqlContentScroll
            anchors.top: sqlPageTitle.bottom
            anchors.topMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: sqlActionBar.top
            anchors.bottomMargin: 12
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: Math.min(sqlContentScroll.availableWidth, 1080)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 14

                Frame {
            Layout.fillWidth: true
            padding: compactLayout ? 14 : 18

            background: Rectangle {
                color: "white"
                radius: 8
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                anchors.fill: parent
                  spacing: 12

                Label {
                    text: "Classes Source Folder"
                    font.pixelSize: 20
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    FolderDialog {
                        id: folderDialog
                        title: "Select script class Folder"
                        onAccepted: {
                            let path = selectedFolder.toString();
                            path = path.replace(/^file:\/\/\//, "");
                            if (path.match(/^\/[a-zA-Z]:\//)) {
                                path = path.substring(1);
                            }
                            lineEdit_scriptclassespath.text = path;
                            appController.setClassesFolderPath(path)
                        }
                    }

                    Button {
                        text: "Browse"
                        font.pixelSize: 15
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 42


                        onClicked: {
                            folderDialog.open()
                        }
                    }

                    TextField {
                        id: lineEdit_scriptclassespath
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42

                        text: appController.classesFolderPath
                        font.pixelSize: 16
                        leftPadding: 18

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter

                        placeholderText: "Classes folder..."

                        onEditingFinished: {
                            appController.setClassesFolderPath(text)
                        }
                    }

                    SettingHelpButton {
                        objectName: "classesFolderHelpButton"
                        helpText: "Chooses the source folder scanned for classes. Only readable source files are used to build the SQL schema prompt."
                    }

                     // Connections {
                     //     target: appController

                     //     function onClassesFolderPathChanged(path)
                     //     {
                     //         lineEdit_scriptclassespath.text = path
                     //     }
                     // }
                }
            }
        }

                Frame {
            Layout.fillWidth: true
            padding: compactLayout ? 14 : 18

            background: Rectangle {
                color: "white"
                radius: 8
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label {
                    text: "SQL Settings"
                    font.pixelSize: 20
                    font.bold: true
                }
                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        id: auditFieldsCheckBox
                        text: "Add audit fields (createdAt, updatedAt)"
                        font.pixelSize: 16
                        Layout.fillWidth: true
                    }

                    SettingHelpButton {
                        objectName: "auditFieldsHelpButton"
                        helpText: "Adds creation and update timestamps for traceability. This supports audits but does not replace access control."
                    }
                }
            }
        }

                Frame {
            Layout.fillWidth: true
                    Layout.preferredHeight: 340
            padding: compactLayout ? 14 : 18

            background: Rectangle {
                color: "white"
                radius: 8
                border.color: "#d0d5dd"
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label {
                    text: "Generated SQL"
                    font.pixelSize: 20
                    font.bold: true
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 250
                    clip: true
                        TextArea {
                            id: lineEdit_generatedsqlcodeoutput
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            font.family: "Consolas"
                            font.pixelSize: 15
                            wrapMode: TextEdit.WrapAnywhere
                            placeholderText: "Generated SQL will appear here..."
                            height: parent.height
                        }
                    }
            }
        }
            }
        }

        GridLayout {
            id: sqlActionBar
            objectName: "sqlActionBar"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            columns: sqlGeneratorPage.width < 560 ? 1 : sqlGeneratorPage.width < 860 ? 2 : 4
            columnSpacing: 14
            rowSpacing: 10

            Connections {
                 target: appController

                //  function onClassesFolderPathChanged(path) {
                //      lineEdit_scriptclassespath.text = path
                // }

                 function onSqlOutputChanged(text) {
                     lineEdit_generatedsqlcodeoutput.text = text
                 }

                 // function onSqlGenerationLoadingChanged(loading) {
                 //     progressBar_loading.visible = loading
                 // }

                 // function onSqlGenerateEnabledChanged(enabled) {
                 //     pushButton_generate.enabled = enabled
                 // }

                 function onWarningOccurred(title, message) {
                     lineEdit_generatedsqlcodeoutput.text = title + "\n" + message
                 }
            }

            Button {
                id: pushButton_generate
                text: "Generate SQL"
                enabled: !appController.loading && appController.aiEnvironmentReady
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                onClicked: {
                    appController.setClassesFolderPath(lineEdit_scriptclassespath.text)
                    appController.setAddAuditFields(auditFieldsCheckBox.checked)
                    appController.onGenerateSqlCode()
                    //button_SqlBack.enabled = false
                   // pushButton_generate.enabled = false
                    //button_execute.enabled = true
                }
            }

            // Button {
            //     text: "Normalize"
            //     font.pixelSize: 16
            //     Layout.preferredHeight: 48
            // }

            Button {
                id: button_execute
                text: "Execute SQL"
                enabled: appController.executable && !appController.loading && lineEdit_generatedsqlcodeoutput.text.length !== 0
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                onClicked: {
                    lineEdit_generatedsqlcodeoutput.text = appController.onExecuteSqlCode(
                        lineEdit_generatedsqlcodeoutput.text
                    )
                }
            }

            ProgressBar {
                id: progressBar_loading
                visible: appController.loading
                indeterminate: true
                Layout.fillWidth: true
                Layout.minimumWidth: 140
            }

            Button {
                id: button_SqlBack
                text: "Back"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 110
                Layout.preferredHeight: 48
                enabled: !appController.loading
                onClicked: {
                    appStack.pop()
                }
            }
        }
    }
}
