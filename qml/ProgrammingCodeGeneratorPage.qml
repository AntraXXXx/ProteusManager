import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    property StackView appStack
    title: "Programming Code Generator"

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 34
        spacing: 22

        Label {
            text: "Programming Code Generator"
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
                            let path = folderDialog.currentFolder.toString();
                            path = path.replace(/^file:\/\/\//, "");
                            if (path.match(/^\/[a-zA-Z]:\//)) {
                                path = path.substring(1);
                            }
                            lineEdit_scriptoutputfolder.text = path;
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
                        Layout.preferredWidth: 750
                        Layout.preferredHeight: 35
                        Layout.alignment: Qt.AlignHCenter

                        font.pixelSize: 16
                        leftPadding: 18

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter

                        placeholderText: "DAL output folder..."
                    }

                    Connections {
                        target: appController

                        function onDalOutputPathChanged(path)
                        {
                            lineEdit_scriptoutputfolder.text = path
                        }
                    }
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
                    text: "Generated DAL"
                    font.pixelSize: 22
                    font.bold: true
                }

                TextArea {
                    id: plainTextEdit_dal
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.family: "Consolas"
                    font.pixelSize: 15
                    wrapMode: TextEdit.WrapAnywhere
                    placeholderText: "Generated database access layer code will appear here..."
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

        ProgressBar {
            Layout.fillWidth: true
            visible: false
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                text: "Generate DAL"
                font.pixelSize: 16
                Layout.preferredHeight: 48
                onClicked: {
                    plainTextEdit_dal.text =
                            "Generating database access layer..."

                    appController.onGenerateDalCode(
                        checkBox_apiaccess.checked
                    )
                }
            }

            Button {
                text: "Export DAL"
                font.pixelSize: 16
                Layout.preferredHeight: 48

                onClicked: {
                    appController.onExportDalCode(
                        plainTextEdit_dal.text,
                        lineEdit_scriptoutputfolder.text
                    )
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: "Back"
                font.pixelSize: 16
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48

                onClicked: {
                    appStack.pop()
                }
            }
        }
    }
}