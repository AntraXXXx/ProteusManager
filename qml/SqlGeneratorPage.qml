import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: sqlGeneratorPage

    property StackView appStack
    readonly property bool compactLayout: width < 700

    title: "Class to SQL"

    background: Rectangle {
        color: "#eef2f7"
    }

    Item {
        anchors.fill: parent
        anchors.margins: compactLayout ? 12 : 24

        ColumnLayout {
            id: sqlPageHeader
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 3

            Label {
                text: "Class to SQL"
                font.pixelSize: 32
                font.bold: true
                color: "#101828"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                objectName: "classToSqlWorkflowLabel"
                text: appController.selectedLanguageName
                      + " classes  ->  database schema"
                font.pixelSize: 14
                color: "#667085"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        ScrollView {
            id: sqlContentScroll
            anchors.top: sqlPageHeader.bottom
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
                        spacing: 10

                        Label {
                            text: "1. Source Classes"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#101828"
                        }

                        Label {
                            objectName: "classSourceRequirementLabel"
                            text: "Expected input: "
                                  + appController.selectedLanguageName
                                  + " class declarations with data fields"
                            color: "#475467"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            FolderDialog {
                                id: folderDialog
                                title: "Select class source folder"
                                onAccepted: {
                                    let path = selectedFolder.toString()
                                    path = path.replace(/^file:\/\/\//, "")
                                    if (path.match(/^\/[a-zA-Z]:\//))
                                        path = path.substring(1)
                                    classSourceFolder.text = path
                                    appController.setClassesFolderPath(path)
                                }
                            }

                            TextField {
                                id: classSourceFolder
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42
                                text: appController.classesFolderPath
                                font.pixelSize: 16
                                leftPadding: 14
                                placeholderText: "Select a folder containing source classes"
                                onEditingFinished: appController.setClassesFolderPath(text)
                            }

                            Button {
                                text: "Browse"
                                font.pixelSize: 15
                                Layout.preferredWidth: 90
                                Layout.preferredHeight: 42
                                onClicked: folderDialog.open()
                            }

                            SettingHelpButton {
                                objectName: "classesFolderHelpButton"
                                helpText: "Scans source files for real class or struct declarations with data fields. Object instances and unrelated files are ignored."
                            }
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
                        spacing: 10

                        Label {
                            text: "2. Schema Options"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#101828"
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
                        spacing: 10

                        Label {
                            text: "3. SQL Preview"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#101828"
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredHeight: 250
                            clip: true

                            TextArea {
                                id: generatedSqlArea
                                font.family: "Consolas"
                                font.pixelSize: 15
                                wrapMode: TextEdit.WrapAnywhere
                                placeholderText: "Generated SQL will appear here..."
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
            columns: sqlGeneratorPage.width < 560
                     ? 1 : sqlGeneratorPage.width < 860 ? 2 : 4
            columnSpacing: 14
            rowSpacing: 10

            Button {
                id: generateSqlButton
                text: "Create SQL Schema"
                enabled: !appController.loading
                         && appController.aiEnvironmentReady
                         && classSourceFolder.text.trim().length > 0
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 150
                Layout.preferredHeight: 48
                onClicked: {
                    appController.setClassesFolderPath(classSourceFolder.text)
                    appController.setAddAuditFields(auditFieldsCheckBox.checked)
                    appController.onGenerateSqlCode()
                }
            }

            Button {
                id: executeSqlButton
                text: "Apply to Database"
                enabled: appController.executable
                         && !appController.loading
                         && generatedSqlArea.text.trim().length > 0
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 150
                Layout.preferredHeight: 48
                onClicked: {
                    generatedSqlArea.text = appController.onExecuteSqlCode(
                                generatedSqlArea.text)
                }
            }

            ProgressBar {
                visible: appController.loading
                indeterminate: true
                Layout.fillWidth: true
                Layout.minimumWidth: 140
            }

            Button {
                text: "Back"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 110
                Layout.preferredHeight: 48
                enabled: !appController.loading
                onClicked: appStack.pop()
            }
        }

        Connections {
            target: appController

            function onSqlOutputChanged(text) {
                generatedSqlArea.text = text
            }

            function onWarningOccurred(title, message) {
                generatedSqlArea.text = title + "\n" + message
            }
        }
    }
}
