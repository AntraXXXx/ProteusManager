import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var tableData

    readonly property int attributeCount: tableData.columns
                                          ? tableData.columns.length
                                          : 0
    readonly property int attributeRowHeight: 28

    implicitHeight: 58 + Math.max(1, attributeCount) * attributeRowHeight + 12
    radius: 6
    color: "white"
    border.width: tableData.proposed ? 2 : 1
    border.color: tableData.proposed ? "#1570ef" : "#98a2b3"

    function keyLabel(columnData) {
        const labels = []
        if (columnData.primaryKey)
            labels.push("PK")
        if (columnData.foreignKey)
            labels.push("FK")
        if (columnData.unique && !columnData.primaryKey)
            labels.push("UQ")
        return labels.join("/")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            color: root.tableData.proposed ? "#175cd3" : "#344054"
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    text: root.tableData.name
                    color: "white"
                    font.bold: true
                    font.pixelSize: 17
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    visible: root.tableData.proposed === true
                    text: "PREVIEW"
                    color: "#dbeafe"
                    font.bold: true
                    font.pixelSize: 10
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            spacing: 0

            Repeater {
                model: Math.max(1, root.attributeCount)

                delegate: Rectangle {
                    required property int index
                    property var columnData: root.attributeCount > 0
                                             ? root.tableData.columns[index]
                                             : null

                    Layout.fillWidth: true
                    Layout.preferredHeight: root.attributeRowHeight
                    color: columnData
                           && (columnData.primaryKey
                               || columnData.foreignKey)
                           ? "#f0f6ff"
                           : index % 2 === 0 ? "#ffffff" : "#f9fafb"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 6

                        Label {
                            text: columnData
                                  ? root.keyLabel(columnData)
                                  : ""
                            color: columnData && columnData.primaryKey
                                   ? "#b54708"
                                   : "#175cd3"
                            font.bold: true
                            font.pixelSize: 10
                            Layout.preferredWidth: 42
                            elide: Text.ElideRight
                        }

                        Label {
                            text: columnData
                                  ? columnData.name
                                  : "No attributes"
                            color: columnData ? "#101828" : "#667085"
                            font.italic: !columnData
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            visible: columnData !== null
                            text: columnData ? columnData.type : ""
                            color: "#475467"
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            Layout.maximumWidth: 90
                        }

                        Label {
                            visible: columnData !== null
                            text: columnData && columnData.nullable
                                  ? "NULL"
                                  : "NOT NULL"
                            color: columnData && columnData.nullable
                                   ? "#667085"
                                   : "#027a48"
                            font.bold: columnData
                                       && !columnData.nullable
                            font.pixelSize: 9
                            horizontalAlignment: Text.AlignRight
                            Layout.preferredWidth: 48
                        }
                    }
                }
            }
        }
    }
}
