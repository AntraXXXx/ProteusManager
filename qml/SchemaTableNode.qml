import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var tableData
    radius: 6
    color: "white"
    border.width: 1
    border.color: tableData.proposed ? "#1570ef" : "#98a2b3"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            color: root.tableData.proposed ? "#175cd3" : "#344054"
            Layout.fillWidth: true
            Layout.preferredHeight: 46

            Label {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                text: root.tableData.name
                color: "white"
                font.bold: true
                font.pixelSize: 17
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            spacing: 3

            Repeater {
                model: Math.min(
                           8,
                           root.tableData.columns
                           ? root.tableData.columns.length
                           : 0)

                delegate: RowLayout {
                    required property int index
                    property var columnData: root.tableData.columns[index]
                    Layout.fillWidth: true
                    Layout.preferredHeight: 21
                    spacing: 6

                    Label {
                        text: columnData.primaryKey
                              ? "PK"
                              : columnData.foreignKey ? "FK" : ""
                        color: columnData.primaryKey ? "#b54708" : "#175cd3"
                        font.bold: true
                        font.pixelSize: 11
                        Layout.preferredWidth: 22
                    }

                    Label {
                        text: columnData.name
                        color: "#101828"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: columnData.type
                        color: "#667085"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                        Layout.maximumWidth: 92
                    }
                }
            }

            Label {
                visible: root.tableData.columns
                         && root.tableData.columns.length > 8
                text: "+ " + (root.tableData.columns.length - 8) + " more columns"
                color: "#667085"
                font.italic: true
                font.pixelSize: 12
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
