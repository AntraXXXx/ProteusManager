import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Page {
    id: normalizationPage
    property StackView appStack

    title: "Database Normalization"

    function openBeforeDiagram() {
        if (appController.refreshNormalizationDiagrams()
                && appController.normalizationBeforeSchema.length > 0) {
            beforeDiagramWindow.visible = true
            beforeDiagramWindow.raise()
            beforeDiagramWindow.requestActivate()
        }
    }

    function openAfterDiagram() {
        appController.refreshNormalizationDiagrams()
        if (appController.normalizationAfterSchema.length > 0) {
            afterDiagramWindow.visible = true
            afterDiagramWindow.raise()
            afterDiagramWindow.requestActivate()
        }
    }

    Window {
        id: beforeDiagramWindow
        objectName: "beforeDiagramWindow"
        visible: false
        flags: Qt.Window
        width: 1100
        height: 720
        minimumWidth: 600
        minimumHeight: 420
        title: "Entity-Relationship Model Before Normalization"
        color: "#eef2f7"

        onClosing: function(close) {
            close.accepted = false
            hide()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Label {
                text: "Entity-Relationship Model Before Normalization"
                color: "#101828"
                font.bold: true
                font.pixelSize: 24
                Layout.fillWidth: true
            }

            Label {
                text: "Original source version | PK = primary key | FK = foreign key | UQ = unique"
                color: "#475467"
                font.pixelSize: 13
                Layout.fillWidth: true
            }

            Label {
                text: "Crow's Foot: 0..* = zero or many | 0..1 = zero or one | 1 = exactly one | solid blue = identifying | dashed gray = non-identifying"
                color: "#475467"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            SchemaDiagramView {
                objectName: "beforeErModel"
                schema: appController.normalizationBeforeSchema
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    Window {
        id: afterDiagramWindow
        objectName: "afterDiagramWindow"
        visible: false
        flags: Qt.Window
        width: 1100
        height: 720
        minimumWidth: 600
        minimumHeight: 420
        title: "Entity-Relationship Model After Normalization"
        color: "#eef2f7"

        onClosing: function(close) {
            close.accepted = false
            hide()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Label {
                text: "Entity-Relationship Model After Normalization"
                color: "#101828"
                font.bold: true
                font.pixelSize: 24
                Layout.fillWidth: true
            }

            Label {
                text: appController.selectedNormalizationForm.length > 0
                      ? "Selected version: " + appController.selectedNormalizationForm
                        + " | Blue entities are proposed by the migration."
                      : "Active version: " + appController.appliedNormalizationForm
                color: "#475467"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: "Crow's Foot: 0..* = zero or many | 0..1 = zero or one | 1 = exactly one | solid blue = identifying | dashed gray = non-identifying"
                color: "#475467"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            SchemaDiagramView {
                objectName: "afterErModel"
                schema: appController.normalizationAfterSchema
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    background: Rectangle {
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "Database Normalization"
            font.pixelSize: 34
            font.bold: true
            color: "#101828"
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            text: appController.appliedNormalizationForm.length > 0
                  ? "Applied: " + appController.appliedNormalizationForm
                  : "Applied: none"
            font.pixelSize: 16
            color: "#475467"
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Frame {
            Layout.fillWidth: true
            padding: 20

            background: Rectangle {
                color: "white"
                radius: 8
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                Label {
                    text: "Target Normal Form"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#101828"
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: normalizationPage.width < 560
                             ? 2
                             : normalizationPage.width < 900 ? 3 : 6
                    columnSpacing: 10
                    rowSpacing: 10

                    Repeater {
                        model: appController.normalizationForms()

                        Button {
                            required property string modelData
                            text: modelData
                            checkable: true
                            checked: appController.selectedNormalizationForm === modelData
                            enabled: !appController.loading
                                     && appController.databaseConnected
                            font.pixelSize: 16
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44

                            onClicked: {
                                appController.onGenerateNormalization(modelData)
                            }
                        }
                    }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    visible: appController.loading
                    indeterminate: true
                }

                Label {
                    text: appController.normalizationStatus
                    font.pixelSize: 15
                    color: appController.normalizationReady ? "#15803d" : "#475467"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    text: appController.selectedNormalizationForm.length > 0
                          ? "Selected target: " + appController.selectedNormalizationForm
                          : "Selected target: none"
                    font.pixelSize: 14
                    color: "#667085"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 20

            background: Rectangle {
                color: "white"
                radius: 8
                border.color: "#d0d5dd"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label {
                    text: "Migration Preview"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#101828"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: migrationOutput
                        text: appController.normalizationOutput
                        readOnly: true
                        selectByMouse: true
                        font.family: "Consolas"
                        font.pixelSize: 14
                        wrapMode: TextEdit.WrapAnywhere
                        placeholderText: "Generated normalization migration will appear here..."
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: normalizationPage.width < 560
                     ? 1
                     : normalizationPage.width < 1000 ? 2 : 6
            columnSpacing: 12
            rowSpacing: 10

            Button {
                objectName: "openBeforeDiagramButton"
                text: "Open Before ER Model"
                enabled: appController.databaseConnected
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    normalizationPage.openBeforeDiagram()
                }
            }

            Button {
                objectName: "openAfterDiagramButton"
                text: "Open After ER Model"
                enabled: appController.databaseConnected
                         && appController.normalizationAfterSchema.length > 0
                         && (appController.normalizationReady
                             || appController.appliedNormalizationForm.length > 0)
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    normalizationPage.openAfterDiagram()
                }
            }

            Button {
                text: "Apply Normalization"
                enabled: appController.normalizationReady
                         && !appController.loading
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    appController.onApplyNormalization()
                }
            }

            Button {
                text: "Previous Level"
                enabled: appController.canResetNormalization
                         && !appController.loading
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    appController.onResetNormalization()
                }
            }

            Button {
                text: "Next Level"
                enabled: appController.canAdvanceNormalization
                         && !appController.loading
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    appController.onAdvanceNormalization()
                }
            }

            Button {
                text: "Back"
                enabled: !appController.loading
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                onClicked: {
                    appStack.pop()
                }
            }
        }
    }
}
