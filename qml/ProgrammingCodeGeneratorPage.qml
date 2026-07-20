import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Page {
    id: codeGeneratorPage

    property StackView appStack
    property bool assistantOpen: false
    property bool advancedSettingsOpen: false
    readonly property var generationProfile: appController.codeGenerationCapabilities
    readonly property var layerSupport: generationProfile.layerSupport || ({})

    title: appController.selectedLanguageName + " Secure Code Generator"

    function modelIndex(values, selectedValue) {
        if (!values)
            return 0

        for (let index = 0; index < values.length; ++index) {
            if (String(values[index]) === String(selectedValue))
                return index
        }

        return 0
    }

    function syncProfileSettings() {
        architectureBox.currentIndex = modelIndex(
                    architectureBox.model,
                    appController.codeGenerationOptions.architecture)
        databaseApiBox.currentIndex = modelIndex(
                    databaseApiBox.model,
                    appController.codeGenerationOptions.databaseApi)
        dataAccessPatternBox.currentIndex = modelIndex(
                    dataAccessPatternBox.model,
                    appController.codeGenerationOptions.dataAccessPattern)
        generationScopeBox.currentIndex = detectedScope()
    }

    function supportedOptionEnabled(optionName, supportName) {
        return layerSupport[supportName] !== true
                || appController.codeGenerationOptions[optionName] === true
    }

    function supportedOptionDisabled(optionName, supportName) {
        return layerSupport[supportName] !== true
                || appController.codeGenerationOptions[optionName] !== true
    }

    function detectedScope() {
        const secureAccess = supportedOptionEnabled("entity", "entity")
                && supportedOptionEnabled("repository", "repository")
                && supportedOptionDisabled("dto", "dto")
                && supportedOptionDisabled("service", "service")
                && supportedOptionDisabled("controller", "controller")
                && supportedOptionDisabled("domainModel", "domainModel")

        if (secureAccess)
            return 0

        const fullApplication = supportedOptionEnabled("entity", "entity")
                && supportedOptionEnabled("dto", "dto")
                && supportedOptionEnabled("repository", "repository")
                && supportedOptionEnabled("service", "service")
                && supportedOptionEnabled("controller", "controller")
                && supportedOptionEnabled("domainModel", "domainModel")

        return fullApplication ? 1 : 2
    }

    function applyGenerationScope(scopeIndex) {
        if (scopeIndex === 2)
            return

        const fullApplication = scopeIndex === 1

        if (layerSupport.entity === true)
            entityCheckBox.checked = true
        if (layerSupport.repository === true)
            repositoryCheckBox.checked = true
        if (layerSupport.dto === true)
            dtoCheckBox.checked = fullApplication
        if (layerSupport.service === true)
            serviceCheckBox.checked = fullApplication
        if (layerSupport.controller === true)
            controllerCheckBox.checked = fullApplication
        if (layerSupport.domainModel === true)
            domainModelCheckBox.checked = fullApplication
        if (generationProfile.supportsInterfaces === true)
            interfacesCheckBox.checked = true
        if (generationProfile.supportsAsyncOperations === true)
            asyncCheckBox.checked = fullApplication
    }

    Component.onCompleted: Qt.callLater(syncProfileSettings)

    Connections {
        target: appController

        function onCodeGenerationSettingsChanged() {
            Qt.callLater(codeGeneratorPage.syncProfileSettings)
        }
    }

    background: Rectangle {
        color: "#eef2f7"
    }

    Item {
        anchors.fill: parent
        anchors.margins: codeGeneratorPage.width < 620 ? 14 : 24

        ColumnLayout {
            id: codePageHeader
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Label {
                    text: appController.selectedLanguageName
                          + " Secure Code Generator"
                    font.pixelSize: 32
                    font.bold: true
                    color: "#101828"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Button {
                    id: assistantToggle
                    text: codeGeneratorPage.assistantOpen
                          ? "Close Assistant"
                          : "Project Assistant"
                    checkable: true
                    checked: codeGeneratorPage.assistantOpen
                    Layout.preferredHeight: 40
                    onToggled: codeGeneratorPage.assistantOpen = checked
                }
            }

            Label {
                objectName: "secureCodeWorkflowLabel"
                text: "Connected database schema  ->  validated "
                      + appController.selectedLanguageName
                      + " access code"
                color: "#667085"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        ScrollView {
            id: dalContentScroll
            anchors.top: codePageHeader.bottom
            anchors.topMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: codeActionBar.top
            anchors.bottomMargin: 12
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: dalContentScroll.availableWidth
                spacing: 14

                Frame {
                    Layout.fillWidth: true
                    padding: 18

                    background: Rectangle {
                        color: "white"
                        radius: 8
                        border.color: "#d0d5dd"
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        Label {
                            text: "Output Directory"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#101828"
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            FolderDialog {
                                id: folderDialog
                                title: "Select output directory"
                                onAccepted: {
                                    let path = selectedFolder.toString()
                                    path = path.replace(/^file:\/\/\//, "")
                                    if (path.match(/^\/[a-zA-Z]:\//))
                                        path = path.substring(1)
                                    outputFolder.text = path
                                    appController.setDalOutputPath(path)
                                }
                            }

                            TextField {
                                id: outputFolder
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                text: appController.dalOutputPath
                                placeholderText: "Output directory"
                                onEditingFinished: appController.setDalOutputPath(text)
                            }

                            Button {
                                text: "Browse"
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 40
                                onClicked: folderDialog.open()
                            }

                            SettingHelpButton {
                                objectName: "outputDirectoryHelpButton"
                                helpText: "Writes exported files to this folder only after the generated code passes security validation."
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    padding: 18

                    background: Rectangle {
                        color: "white"
                        radius: 8
                        border.color: "#d0d5dd"
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            text: "Security Profile"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#101828"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: generationProfile.summary || "Secure defaults are ready for the selected language."
                            color: "#475467"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: codeGeneratorPage.width < 720 ? 1 : 2
                            columnSpacing: 24
                            rowSpacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 9

                                Rectangle {
                                    Layout.preferredWidth: 9
                                    Layout.preferredHeight: 9
                                    radius: 5
                                    color: "#16a34a"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Label {
                                        text: "SQL injection protection"
                                        font.bold: true
                                        color: "#101828"
                                    }

                                    Label {
                                        text: "Parameterized queries are always enabled"
                                        color: "#067647"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 9

                                Rectangle {
                                    Layout.preferredWidth: 9
                                    Layout.preferredHeight: 9
                                    radius: 5
                                    color: "#16a34a"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Label {
                                        text: "Export protection"
                                        font.bold: true
                                        color: "#101828"
                                    }

                                    Label {
                                        text: "Invalid generated code cannot be exported"
                                        color: "#067647"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: codeGeneratorPage.width < 720 ? 1 : 2
                            columnSpacing: 24
                            rowSpacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: "Generation Scope"
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }

                                    SettingHelpButton {
                                        objectName: "generationScopeHelpButton"
                                        helpText: "Secure data access generates the database-facing pieces. Full application layers also add business and presentation code."
                                    }
                                }

                                ComboBox {
                                    id: generationScopeBox
                                    objectName: "generationScopeBox"
                                    Layout.fillWidth: true
                                    model: [
                                        "Secure data access",
                                        "Full application layers",
                                        "Custom configuration"
                                    ]
                                    onActivated: function(index) {
                                        codeGeneratorPage.applyGenerationScope(index)
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                CheckBox {
                                    id: unitTestsCheckBox
                                    text: "Generate security-focused tests"
                                    checked: appController.codeGenerationOptions.unitTests
                                    Layout.fillWidth: true
                                }

                                SettingHelpButton {
                                    helpText: "Adds tests for mappings, input handling and repository contracts without production credentials."
                                }
                            }
                        }

                        ToolButton {
                            id: advancedSettingsToggle
                            objectName: "advancedSettingsToggle"
                            text: codeGeneratorPage.advancedSettingsOpen
                                  ? "Hide advanced settings"
                                  : "Show advanced settings"
                            checkable: true
                            checked: codeGeneratorPage.advancedSettingsOpen
                            onToggled: codeGeneratorPage.advancedSettingsOpen = checked
                            Accessible.name: text
                        }

                        GridLayout {
                            id: advancedSettingsGrid
                            objectName: "advancedSettingsGrid"
                            visible: codeGeneratorPage.advancedSettingsOpen
                            Layout.fillWidth: true
                            columns: codeGeneratorPage.width < 780 ? 1 : 2
                            columnSpacing: 28
                            rowSpacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        text: "Architecture"
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Controls how dependencies flow between generated layers. Choose the simplest option that fits the project."
                                    }
                                }

                                ComboBox {
                                    id: architectureBox
                                    Layout.fillWidth: true
                                    model: generationProfile.architectures || []
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    Label {
                                        text: "Database API"
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Selects the database library and its language-specific resource and parameter-binding rules."
                                    }
                                }

                                ComboBox {
                                    id: databaseApiBox
                                    Layout.fillWidth: true
                                    model: generationProfile.databaseApis || []
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    Label {
                                        text: "Data Access Pattern"
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Defines how database operations are isolated from business logic in this language."
                                    }
                                }

                                ComboBox {
                                    id: dataAccessPatternBox
                                    Layout.fillWidth: true
                                    model: generationProfile.dataAccessPatterns || []
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: generationProfile.supportsInterfaces === true
                                    CheckBox {
                                        id: interfacesCheckBox
                                        text: generationProfile.interfaceLabel || "Generate interfaces"
                                        checked: appController.codeGenerationOptions.interfaces
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Creates the language-appropriate abstraction, such as an interface, protocol or trait, for easier testing and replacement."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: generationProfile.supportsAsyncOperations === true
                                    CheckBox {
                                        id: asyncCheckBox
                                        text: generationProfile.asyncLabel || "Generate asynchronous operations"
                                        checked: appController.codeGenerationOptions.asyncOperations
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Uses the language's supported non-blocking or context-aware database workflow without fake blocking wrappers."
                                    }
                                }

                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    text: "Layers"
                                    font.bold: true
                                    color: "#101828"
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.entity === true
                                    CheckBox {
                                        id: entityCheckBox
                                        text: "Entity"
                                        checked: appController.codeGenerationOptions.entity
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Represents one persisted database row and contains no database calls."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.dto === true
                                    CheckBox {
                                        id: dtoCheckBox
                                        text: "DTO"
                                        checked: appController.codeGenerationOptions.dto
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Transfers selected data between layers without SQL or business logic."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.repository === true
                                    CheckBox {
                                        id: repositoryCheckBox
                                        text: "Repository / DAO"
                                        checked: appController.codeGenerationOptions.repository
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Contains the parameterized database operations and keeps SQL out of the other layers."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.service === true
                                    CheckBox {
                                        id: serviceCheckBox
                                        text: "Service / BLL"
                                        checked: appController.codeGenerationOptions.service
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Coordinates business workflows and depends on data-access abstractions."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.controller === true
                                    CheckBox {
                                        id: controllerCheckBox
                                        text: "Controller / Presentation"
                                        checked: appController.codeGenerationOptions.controller
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Validates external input, calls services and maps responses without issuing SQL."
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: layerSupport.domainModel === true
                                    CheckBox {
                                        id: domainModelCheckBox
                                        text: "Domain Model"
                                        checked: appController.codeGenerationOptions.domainModel
                                        Layout.fillWidth: true
                                    }
                                    SettingHelpButton {
                                        helpText: "Holds business behavior and rules while remaining independent of database drivers."
                                    }
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: codeGeneratorPage.assistantOpen
                                            && codeGeneratorPage.width < 900
                                            ? 760 : 470
                    padding: 0

                    background: Rectangle {
                        color: "white"
                        radius: 8
                        border.color: "#d0d5dd"
                    }

                    GridLayout {
                        anchors.fill: parent
                        columns: codeGeneratorPage.assistantOpen
                                 && codeGeneratorPage.width >= 900 ? 2 : 1
                        columnSpacing: 0
                        rowSpacing: 0

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumWidth: 320
                            Layout.margins: 18
                            spacing: 10

                            Label {
                                text: "Secure Code Preview"
                                font.pixelSize: 20
                                font.bold: true
                                color: "#101828"
                                Layout.fillWidth: true
                            }

                            Label {
                                id: codeGenerationStatus
                                Layout.fillWidth: true
                                text: appController.codeGenerationValidationSummary
                                color: appController.generatedCodeValid ? "#067647" : "#475467"
                                wrapMode: Text.WordWrap
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                TextArea {
                                    id: generatedCodeArea
                                    font.family: "Consolas"
                                    font.pixelSize: 14
                                    wrapMode: TextEdit.WrapAnywhere
                                    placeholderText: "Validated secure code will appear here..."
                                    onTextChanged: {
                                        if (!appController.loading)
                                            appController.validateGeneratedCode(text)
                                    }
                                }
                            }
                        }

                        Rectangle {
                            visible: codeGeneratorPage.assistantOpen
                            Layout.fillWidth: codeGeneratorPage.width < 900
                            Layout.fillHeight: true
                            Layout.preferredWidth: 360
                            Layout.minimumWidth: codeGeneratorPage.width < 900 ? 0 : 300
                            Layout.minimumHeight: codeGeneratorPage.width < 900 ? 330 : 0
                            color: "#f8fafc"
                            border.color: "#d0d5dd"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: "Project Assistant"
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: "#101828"
                                        Layout.fillWidth: true
                                    }

                                    ToolButton {
                                        text: "Clear"
                                        enabled: !appController.codeAssistantBusy
                                                 && appController.codeAssistantMessages.length > 0
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Clear this conversation"
                                        onClicked: appController.clearCodeAssistant()
                                    }

                                    ToolButton {
                                        text: "Close"
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Close assistant"
                                        onClicked: codeGeneratorPage.assistantOpen = false
                                    }
                                }

                                ListView {
                                    id: assistantMessages
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    spacing: 8
                                    clip: true
                                    model: appController.codeAssistantMessages

                                    onCountChanged: positionViewAtEnd()

                                    delegate: Rectangle {
                                        required property var modelData
                                        width: assistantMessages.width
                                        height: assistantMessage.implicitHeight + 20
                                        radius: 6
                                        color: modelData.role === "user"
                                               ? "#e8f1ff" : "white"
                                        border.color: modelData.role === "user"
                                                      ? "#84adff" : "#d0d5dd"

                                        Label {
                                            id: assistantMessage
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            text: modelData.text
                                            color: "#101828"
                                            wrapMode: Text.WordWrap
                                            verticalAlignment: Text.AlignTop
                                        }
                                    }
                                }

                                ProgressBar {
                                    visible: appController.codeAssistantBusy
                                    indeterminate: true
                                    Layout.fillWidth: true
                                }

                                TextArea {
                                    id: assistantQuestion
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 78
                                    placeholderText: "Ask about this project..."
                                    wrapMode: TextEdit.WordWrap
                                    enabled: !appController.codeAssistantBusy
                                }

                                Button {
                                    text: "Send"
                                    Layout.fillWidth: true
                                    enabled: !appController.codeAssistantBusy
                                             && assistantQuestion.text.trim().length > 0
                                    onClicked: {
                                        appController.askCodeAssistant(
                                                    assistantQuestion.text,
                                                    generatedCodeArea.text)
                                        assistantQuestion.clear()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            id: codeActionBar
            objectName: "codeActionBar"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            columns: codeGeneratorPage.width < 700
                     ? 1 : codeGeneratorPage.width < 1000 ? 2 : 4
            columnSpacing: 12
            rowSpacing: 10

            Button {
                id: generateButton
                text: "Generate Secure Code"
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 46
                enabled: !appController.loading && appController.aiEnvironmentReady

                onClicked: {
                    generatedCodeArea.text = "Generating secure application code..."
                    appController.onGenerateApplicationCode({
                        "architecture": architectureBox.currentText,
                        "databaseApi": databaseApiBox.currentText,
                        "dataAccessPattern": dataAccessPatternBox.currentText,
                        "entity": layerSupport.entity === true && entityCheckBox.checked,
                        "dto": layerSupport.dto === true && dtoCheckBox.checked,
                        "repository": layerSupport.repository === true && repositoryCheckBox.checked,
                        "service": layerSupport.service === true && serviceCheckBox.checked,
                        "controller": layerSupport.controller === true && controllerCheckBox.checked,
                        "domainModel": layerSupport.domainModel === true && domainModelCheckBox.checked,
                        "interfaces": generationProfile.supportsInterfaces === true && interfacesCheckBox.checked,
                        "asyncOperations": generationProfile.supportsAsyncOperations === true && asyncCheckBox.checked,
                        "unitTests": unitTestsCheckBox.checked
                    })
                }
            }

            Button {
                text: "Export Code"
                Layout.fillWidth: true
                Layout.minimumWidth: 140
                Layout.preferredHeight: 46
                enabled: !appController.loading
                         && appController.generatedCodeValid
                         && outputFolder.text.length > 0
                onClicked: appController.onExportDalCode(
                               generatedCodeArea.text,
                               outputFolder.text)
            }

            ProgressBar {
                visible: appController.loading
                indeterminate: true
                Layout.fillWidth: true
                Layout.minimumWidth: 140
            }

            Button {
                text: "Back"
                Layout.fillWidth: true
                Layout.minimumWidth: 110
                Layout.preferredHeight: 46
                enabled: !appController.loading
                onClicked: appStack.pop()
            }
        }

        Connections {
            target: appController

            function onDalOutputChanged(code) {
                generatedCodeArea.text = code
            }

            function onDalStatusChanged(status) {
                codeGenerationStatus.text = status
            }
        }
    }
}
