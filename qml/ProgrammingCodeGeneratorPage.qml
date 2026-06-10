import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: codeGeneratorPage
    property StackView appStack
    title: appController.selectedLanguageName + " Code Generator"

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: appController.selectedLanguageName + " Code Generator"
            font.pixelSize: 36
            font.bold: true
            color: "#101828"
            Layout.alignment: Qt.AlignHCenter
        }

        ScrollView {
            id: dalContentScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: dalContentScroll.availableWidth
                spacing: 16

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
                    text: "Output Directory"
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
                            lineEdit_scriptoutputfolder.text = path;
                            appController.setDalOutputPath(path)
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
                        id: lineEdit_scriptoutputfolder
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 700
                        Layout.alignment: Qt.AlignHCenter
                        text: appController.dalOutputPath
                        font.pixelSize: 16
                        leftPadding: 18

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter

                        onTextChanged: {
                            appController.setDalOutputPath(text)
                        }

                        placeholderText: "DAL output folder..."
                    }

                    // Connections {
                    //     target: appController

                    //     function dalOutputChanged(path)
                    //     {
                    //         lineEdit_scriptoutputfolder.text = path
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
                    text: "DAL Settings:"
                    font.pixelSize: 22
                    font.bold: true
                }
                GroupBox {
                    CheckBox {
                        id: checkBox_apiaccess
                        text: "Generate secure DAL"
                        font.pixelSize: 16
                        checked: true
                    }
                }
            }
        }

                Frame {
            Layout.fillWidth: true
                    Layout.preferredHeight: 340
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
                    text: "Generated DAL"
                    font.pixelSize: 22
                    font.bold: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 250
                    clip: true
                        TextArea {
                            id: plainTextEdit_dal
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            font.family: "Consolas"
                            font.pixelSize: 15
                            wrapMode: TextEdit.WrapAnywhere
                            placeholderText: "Generated database access layer code will appear here..."
                            height: parent.height
                        }
                    }

                Connections {
                    target: appController

                    function onDalOutputChanged(code) {
                        plainTextEdit_dal.text = code
                    }

                    function onDalStatusChanged(status) {
                        plainTextEdit_dal.text = status
                    }
                }
            }
        }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: false
        }

        GridLayout {
            Layout.fillWidth: true
            columns: codeGeneratorPage.width < 560 ? 1 : codeGeneratorPage.width < 860 ? 2 : 4
            columnSpacing: 14
            rowSpacing: 10

            Button {
                id: button_generateDal
                text: "Generate DAL"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                enabled: !appController.loading && appController.aiEnvironmentReady
                onClicked: {
                    plainTextEdit_dal.text =
                            "Generating database access layer..."

                    appController.onGenerateDalCode(
                        checkBox_apiaccess.checked
                    )
                   // button_dalBack.enabled = !appController.loading
                    //button_generateDal.enabled = !appController.loading
                }
            }

            Button {
                id: button_executeDal
                text: "Export DAL"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                enabled: !appController.loading
                         && plainTextEdit_dal.text.indexOf("FILE:") !== -1
                         && lineEdit_scriptoutputfolder.text.length > 0
                onClicked: {
                    appController.onExportDalCode(
                        plainTextEdit_dal.text,
                        lineEdit_scriptoutputfolder.text
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
                id: button_dalBack
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
