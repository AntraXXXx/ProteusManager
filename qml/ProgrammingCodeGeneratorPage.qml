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
                anchors.fill: parent
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
                        Layout.preferredHeight: 42

                        font.pixelSize: 16
                        leftPadding: 18

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter
                        text: appController.dalOutputPath


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
                    text: "Code Generation Settings"
                    font.pixelSize: 22
                    font.bold: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: codeGeneratorPage.width < 760 ? 1 : 2
                    columnSpacing: 24
                    rowSpacing: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Architecture"
                            font.bold: true
                        }

                        ComboBox {
                            id: architectureBox
                            Layout.fillWidth: true
                            model: ["Layered", "Clean Architecture", "Hexagonal"]
                            Component.onCompleted: {
                                const saved = appController.codeGenerationOptions.architecture
                                currentIndex = Math.max(0, model.indexOf(saved))
                            }
                        }

                        Label {
                            text: "Data Access Pattern"
                            font.bold: true
                            Layout.topMargin: 6
                        }

                        ComboBox {
                            id: dataAccessPatternBox
                            Layout.fillWidth: true
                            model: ["Repository", "DAO"]
                            Component.onCompleted: {
                                const saved = appController.codeGenerationOptions.dataAccessPattern
                                currentIndex = Math.max(0, model.indexOf(saved))
                            }
                        }

                        CheckBox {
                            text: "Secure parameterized queries"
                            checked: true
                            enabled: false
                        }

                        CheckBox {
                            id: interfacesCheckBox
                            text: "Generate interfaces"
                            checked: appController.codeGenerationOptions.interfaces
                        }

                        CheckBox {
                            id: asyncCheckBox
                            text: "Generate asynchronous operations"
                            checked: appController.codeGenerationOptions.asyncOperations
                        }

                        CheckBox {
                            id: unitTestsCheckBox
                            text: "Generate unit tests"
                            checked: appController.codeGenerationOptions.unitTests
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "Layers"
                            font.bold: true
                        }

                        CheckBox {
                            id: entityCheckBox
                            text: "Entity"
                            checked: appController.codeGenerationOptions.entity
                        }

                        CheckBox {
                            id: dtoCheckBox
                            text: "DTO"
                            checked: appController.codeGenerationOptions.dto
                        }

                        CheckBox {
                            id: repositoryCheckBox
                            text: "Repository / DAO"
                            checked: appController.codeGenerationOptions.repository
                        }

                        CheckBox {
                            id: serviceCheckBox
                            text: "Service / BLL"
                            checked: appController.codeGenerationOptions.service
                        }

                        CheckBox {
                            id: controllerCheckBox
                            text: "Controller / Presentation"
                            checked: appController.codeGenerationOptions.controller
                        }

                        CheckBox {
                            id: domainModelCheckBox
                            text: "Domain Model"
                            checked: appController.codeGenerationOptions.domainModel
                        }
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
                    text: "Generated Code"
                    font.pixelSize: 22
                    font.bold: true
                }

                Label {
                    id: codeGenerationStatus
                    Layout.fillWidth: true
                    text: appController.codeGenerationValidationSummary
                    color: appController.generatedCodeValid ? "#067647" : "#475467"
                    wrapMode: Text.Wrap
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
                            placeholderText: "Generated application code will appear here..."
                            height: parent.height
                            onTextChanged: {
                                if (!appController.loading) {
                                    appController.validateGeneratedCode(text)
                                }
                            }
                        }
                    }

                Connections {
                    target: appController

                    function onDalOutputChanged(code) {
                        plainTextEdit_dal.text = code
                    }

                    function onDalStatusChanged(status) {
                        codeGenerationStatus.text = status
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
                text: "Generate Code"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                enabled: !appController.loading && appController.aiEnvironmentReady
                onClicked: {
                    plainTextEdit_dal.text =
                            "Generating database access layer..."

                    appController.onGenerateApplicationCode({
                        "architecture": architectureBox.currentText,
                        "dataAccessPattern": dataAccessPatternBox.currentText,
                        "entity": entityCheckBox.checked,
                        "dto": dtoCheckBox.checked,
                        "repository": repositoryCheckBox.checked,
                        "service": serviceCheckBox.checked,
                        "controller": controllerCheckBox.checked,
                        "domainModel": domainModelCheckBox.checked,
                        "interfaces": interfacesCheckBox.checked,
                        "asyncOperations": asyncCheckBox.checked,
                        "unitTests": unitTestsCheckBox.checked
                    })
                   // button_dalBack.enabled = !appController.loading
                    //button_generateDal.enabled = !appController.loading
                }
            }

            Button {
                id: button_executeDal
                text: "Export Code"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 48
                enabled: !appController.loading
                         && appController.generatedCodeValid
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
