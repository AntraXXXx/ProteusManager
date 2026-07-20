import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    required property string helpText

    text: "i"
    checkable: true
    hoverEnabled: true
    font.bold: true
    font.pixelSize: 13
    implicitWidth: 26
    implicitHeight: 26

    ToolTip.visible: hovered || checked || activeFocus
    ToolTip.delay: 250
    ToolTip.timeout: 10000
    ToolTip.text: helpText

    background: Rectangle {
        radius: width / 2
        color: root.checked || root.hovered ? "#e8f1ff" : "#f2f4f7"
        border.color: root.checked || root.hovered ? "#1570ef" : "#98a2b3"
    }

    contentItem: Label {
        text: root.text
        color: "#175cd3"
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
