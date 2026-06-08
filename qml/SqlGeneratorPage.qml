import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    property StackView appStack
    title: "SQL Generator"

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 34
        spacing: 22

        Label {
            text: "SQL Generator"
            font.pixelSize: 36
            font.bold: true
            color: "#101828"
            Layout.alignment: Qt.AlignHCenter
        }

        Frame {
            Layout.fillWidth: true
            padding: 24

            background: Rectangle {
                color: "white"
                radius: 18
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                spacing: 14

                Label {
                    text: "Classes Source-Folder"
                    font.pixelSize: 22
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
                        text: "Add"
                        font.pixelSize: 16
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 42


                        onClicked: {
                            folderDialog.open()
                        }
                    }

                    TextField {
                        id: lineEdit_scriptclassespath
                        Layout.preferredWidth: 750
                        Layout.preferredHeight: 35
                        Layout.alignment: Qt.AlignHCenter
                        text: appController.classesFolderPath
                        font.pixelSize: 16
                        leftPadding: 18

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter
                        placeholderText: "Classes folder..."

                        onTextChanged: {
                            appController.setClassesFolderPath(text)
                        }
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
            padding: 20

            background: Rectangle {
                color: "white"
                radius: 18
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label {
                    text: "SQL Settings:"
                    font.pixelSize: 22
                    font.bold: true
                }
                GroupBox {
                    CheckBox {
                        id: auditFieldsCheckBox
                        text: "Add audit fields (createdAt, updatedAt)"
                        font.pixelSize: 16
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 24

            background: Rectangle {
                color: "white"
                radius: 18
                border.color: "#d0d5dd"
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label {
                    text: "Generated SQL"
                    font.pixelSize: 22
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
        ProgressBar {
            Layout.fillWidth: true
            visible: false
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

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
                enabled: appController.executable && !appController.loading
                font.pixelSize: 16
                Layout.preferredHeight: 48
                onClicked: {
                    appController.setClassesFolderPath(lineEdit_scriptclassespath.text)
                    appController.setAddAuditFields(auditFieldsCheckBox.checked)
                    appController.onGenerateSqlCode()
                    button_SqlBack.enabled = false
                   // pushButton_generate.enabled = false
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
                Layout.preferredHeight: 48
                onClicked: {
                    appController.onExecuteSqlCode(
                        lineEdit_generatedsqlcodeoutput.text
                    )
                }
            }

            ProgressBar {
                id: progressBar_loading
                visible: appController.executable && appController.loading
                indeterminate: true
                Layout.preferredWidth: 600
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                id: button_SqlBack
                text: "Back"
                font.pixelSize: 16
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                enabled: appController.executable && appController.loading && lineEdit_generatedsqlcodeoutput.text.length !== 0
                onClicked: {
                    appStack.pop()
                }
            }
        }
    }
}