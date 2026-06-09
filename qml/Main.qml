import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1000
    height: 700
    title: "ProteusManager"

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: MainMenuPage {
            appStack: stackView
        }
    }
}