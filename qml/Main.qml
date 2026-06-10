import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1400
    height: 900
    minimumWidth: 420
    minimumHeight: 520
    title: "ProteusManager"

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: MainMenuPage {
            appStack: stackView
        }
    }
}
