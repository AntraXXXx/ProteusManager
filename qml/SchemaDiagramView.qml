import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var schema: []
    readonly property int nodeWidth: 270
    readonly property int nodeHeight: 260
    readonly property int gapX: 80
    readonly property int gapY: 70
    readonly property int padding: 40
    readonly property int tableCount: schema ? schema.length : 0
    readonly property int columnCount: Math.max(
                                           1,
                                           Math.floor(
                                               (Math.max(width, nodeWidth + 2 * padding)
                                                - 2 * padding + gapX)
                                               / (nodeWidth + gapX)))
    readonly property int rowCount: Math.ceil(tableCount / columnCount)

    function nodeX(index) {
        return padding + (index % columnCount) * (nodeWidth + gapX)
    }

    function nodeY(index) {
        return padding + Math.floor(index / columnCount) * (nodeHeight + gapY)
    }

    function tableIndex(tableName) {
        for (let i = 0; i < tableCount; ++i) {
            if (String(schema[i].name).toLowerCase()
                    === String(tableName).toLowerCase())
                return i
        }
        return -1
    }

    onSchemaChanged: relationCanvas.requestPaint()
    onColumnCountChanged: relationCanvas.requestPaint()

    Rectangle {
        anchors.fill: parent
        color: "#f8fafc"
        border.color: "#d0d5dd"
    }

    Label {
        anchors.centerIn: parent
        visible: root.tableCount === 0
        text: "No schema is available for this database."
        color: "#667085"
        font.pixelSize: 16
    }

    Flickable {
        id: viewport
        anchors.fill: parent
        visible: root.tableCount > 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: diagramSurface.width
        contentHeight: diagramSurface.height
        ScrollBar.vertical: ScrollBar { }
        ScrollBar.horizontal: ScrollBar { }

        Item {
            id: diagramSurface
            width: Math.max(
                       viewport.width,
                       2 * root.padding + root.columnCount * root.nodeWidth
                       + Math.max(0, root.columnCount - 1) * root.gapX)
            height: Math.max(
                        viewport.height,
                        2 * root.padding + root.rowCount * root.nodeHeight
                        + Math.max(0, root.rowCount - 1) * root.gapY)

            Canvas {
                id: relationCanvas
                anchors.fill: parent
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onPaint: root.paintRelations(getContext("2d"))
            }

            Repeater {
                model: root.schema || []
                delegate: SchemaTableNode {
                    required property int index
                    required property var modelData
                    x: root.nodeX(index)
                    y: root.nodeY(index)
                    width: root.nodeWidth
                    height: root.nodeHeight
                    tableData: modelData
                }
            }
        }
    }

    function paintRelations(context) {
        context.reset()
        context.lineWidth = 2
        context.strokeStyle = "#667085"
        context.fillStyle = "#667085"

        for (let source = 0; source < tableCount; ++source) {
            const relations = schema[source].relations || []
            for (let relation = 0; relation < relations.length; ++relation) {
                const target = tableIndex(relations[relation].referenceTable)
                if (target < 0 || target === source)
                    continue

                const sourceX = nodeX(source) + nodeWidth / 2
                const sourceY = nodeY(source) + nodeHeight / 2
                const targetX = nodeX(target) + nodeWidth / 2
                const targetY = nodeY(target) + nodeHeight / 2
                const horizontal = Math.abs(targetX - sourceX)
                                   >= Math.abs(targetY - sourceY)
                const direction = horizontal
                                  ? (targetX > sourceX ? 1 : -1)
                                  : (targetY > sourceY ? 1 : -1)
                const startX = sourceX + (horizontal ? direction * nodeWidth / 2 : 0)
                const startY = sourceY + (horizontal ? 0 : direction * nodeHeight / 2)
                const endX = targetX - (horizontal ? direction * nodeWidth / 2 : 0)
                const endY = targetY - (horizontal ? 0 : direction * nodeHeight / 2)

                context.beginPath()
                context.moveTo(startX, startY)
                if (horizontal) {
                    const middleX = (startX + endX) / 2
                    context.lineTo(middleX, startY)
                    context.lineTo(middleX, endY)
                } else {
                    const middleY = (startY + endY) / 2
                    context.lineTo(startX, middleY)
                    context.lineTo(endX, middleY)
                }
                context.lineTo(endX, endY)
                context.stroke()

                context.beginPath()
                context.arc(endX, endY, 5, 0, 2 * Math.PI)
                context.fill()
            }
        }
    }
}
