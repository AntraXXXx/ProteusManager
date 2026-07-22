import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var schema: []

    readonly property int nodeWidth: 340
    readonly property int attributeRowHeight: 28
    readonly property int gapX: 150
    readonly property int gapY: 110
    readonly property int padding: 60
    readonly property int tableCount: schema ? schema.length : 0
    readonly property int relationshipCount: countRelationships()
    readonly property int columnCount: Math.max(
                                           1,
                                           Math.floor(
                                               (Math.max(width,
                                                         nodeWidth
                                                         + 2 * padding)
                                                - 2 * padding + gapX)
                                               / (nodeWidth + gapX)))
    readonly property int rowCount: Math.ceil(tableCount / columnCount)
    readonly property var orderedIndices: calculateLayoutOrder()
    readonly property real surfaceWidth: Math.max(
                                             width,
                                             2 * padding
                                             + columnCount * nodeWidth
                                             + Math.max(0,
                                                        columnCount - 1)
                                             * gapX)
    readonly property real surfaceHeight: Math.max(
                                              height,
                                              calculateSurfaceHeight())

    function countRelationships() {
        let count = 0
        for (let i = 0; i < tableCount; ++i)
            count += (schema[i].relations || []).length
        return count
    }

    function calculateLayoutOrder() {
        const order = []
        const scores = []
        for (let i = 0; i < tableCount; ++i) {
            order.push(i)
            scores.push(0)
        }

        for (let source = 0; source < tableCount; ++source) {
            const relations = schema[source].relations || []
            for (let relationIndex = 0;
                 relationIndex < relations.length;
                 ++relationIndex) {
                const target = tableIndex(
                                 relations[relationIndex].referenceTable)
                if (target < 0 || target === source)
                    continue
                scores[target] -= 1
                scores[source] += 1
            }
        }

        order.sort(function(left, right) {
            if (scores[left] !== scores[right])
                return scores[left] - scores[right]
            return String(schema[left].name).localeCompare(
                        String(schema[right].name))
        })
        return order
    }

    function displayIndex(schemaIndex) {
        const index = orderedIndices.indexOf(schemaIndex)
        return index < 0 ? schemaIndex : index
    }

    function nodeHeight(schemaIndex) {
        const columns = schema[schemaIndex]
                        && schema[schemaIndex].columns
                        ? schema[schemaIndex].columns.length
                        : 0
        return 70 + Math.max(1, columns) * attributeRowHeight
    }

    function rowHeight(row) {
        let maximum = 0
        const first = row * columnCount
        const last = Math.min(tableCount, first + columnCount)
        for (let position = first; position < last; ++position)
            maximum = Math.max(
                        maximum,
                        nodeHeight(orderedIndices[position]))
        return maximum
    }

    function nodeX(schemaIndex) {
        const position = displayIndex(schemaIndex)
        return padding
                + (position % columnCount) * (nodeWidth + gapX)
    }

    function nodeY(schemaIndex) {
        const position = displayIndex(schemaIndex)
        const row = Math.floor(position / columnCount)
        let y = padding
        for (let previousRow = 0;
             previousRow < row;
             ++previousRow)
            y += rowHeight(previousRow) + gapY
        return y
    }

    function calculateSurfaceHeight() {
        let contentHeight = 2 * padding
        for (let row = 0; row < rowCount; ++row) {
            contentHeight += rowHeight(row)
            if (row + 1 < rowCount)
                contentHeight += gapY
        }
        return contentHeight
    }

    function tableIndex(tableName) {
        for (let i = 0; i < tableCount; ++i) {
            if (String(schema[i].name).toLowerCase()
                    === String(tableName).toLowerCase())
                return i
        }
        return -1
    }

    function relationGeometry(source, target, lane) {
        const sourceX = nodeX(source) + nodeWidth / 2
        const sourceY = nodeY(source) + nodeHeight(source) / 2
        const targetX = nodeX(target) + nodeWidth / 2
        const targetY = nodeY(target) + nodeHeight(target) / 2

        if (source === target) {
            const edgeX = nodeX(source) + nodeWidth
            const loopX = edgeX + 54 + Math.abs(lane)
            return {
                points: [
                    {x: edgeX, y: sourceY - 20},
                    {x: loopX, y: sourceY - 20},
                    {x: loopX, y: sourceY + 20},
                    {x: edgeX, y: sourceY + 20}
                ],
                sourceDx: 1,
                sourceDy: 0,
                targetDx: 1,
                targetDy: 0,
                labelX: loopX,
                labelY: sourceY
            }
        }

        const horizontal = Math.abs(targetX - sourceX)
                           >= Math.abs(targetY - sourceY)
        if (horizontal) {
            const direction = targetX > sourceX ? 1 : -1
            const startX = sourceX + direction * nodeWidth / 2
            const endX = targetX - direction * nodeWidth / 2
            const middleX = (startX + endX) / 2 + lane
            return {
                points: [
                    {x: startX, y: sourceY},
                    {x: middleX, y: sourceY},
                    {x: middleX, y: targetY},
                    {x: endX, y: targetY}
                ],
                sourceDx: direction,
                sourceDy: 0,
                targetDx: -direction,
                targetDy: 0,
                labelX: middleX,
                labelY: (sourceY + targetY) / 2
            }
        }

        const direction = targetY > sourceY ? 1 : -1
        const startY = sourceY + direction * nodeHeight(source) / 2
        const endY = targetY - direction * nodeHeight(target) / 2
        const middleY = (startY + endY) / 2 + lane
        return {
            points: [
                {x: sourceX, y: startY},
                {x: sourceX, y: middleY},
                {x: targetX, y: middleY},
                {x: targetX, y: endY}
            ],
            sourceDx: 0,
            sourceDy: direction,
            targetDx: 0,
            targetDy: -direction,
            labelX: (sourceX + targetX) / 2,
            labelY: middleY
        }
    }

    function drawBar(context, x, y, dx, dy, distance) {
        const centerX = x + dx * distance
        const centerY = y + dy * distance
        const perpendicularX = -dy
        const perpendicularY = dx
        context.beginPath()
        context.moveTo(centerX + perpendicularX * 7,
                       centerY + perpendicularY * 7)
        context.lineTo(centerX - perpendicularX * 7,
                       centerY - perpendicularY * 7)
        context.stroke()
    }

    function drawCircle(context, x, y, dx, dy, distance) {
        context.beginPath()
        context.arc(x + dx * distance,
                    y + dy * distance,
                    5,
                    0,
                    Math.PI * 2)
        context.fillStyle = "#f8fafc"
        context.fill()
        context.stroke()
    }

    function drawCrowFoot(context, x, y, dx, dy) {
        const vertexX = x + dx * 18
        const vertexY = y + dy * 18
        const perpendicularX = -dy
        const perpendicularY = dx

        context.beginPath()
        context.moveTo(vertexX, vertexY)
        context.lineTo(x + perpendicularX * 8,
                       y + perpendicularY * 8)
        context.moveTo(vertexX, vertexY)
        context.lineTo(x, y)
        context.moveTo(vertexX, vertexY)
        context.lineTo(x - perpendicularX * 8,
                       y - perpendicularY * 8)
        context.stroke()
    }

    function drawCardinality(context, x, y, dx, dy, cardinality) {
        const value = String(cardinality || "0..*")
        if (value.indexOf("*") >= 0)
            drawCrowFoot(context, x, y, dx, dy)
        else
            drawBar(context, x, y, dx, dy, 9)

        if (value.indexOf("0..") === 0)
            drawCircle(context, x, y, dx, dy, 29)
        else
            drawBar(context, x, y, dx, dy, 27)
    }

    function compactRelationLabel(source, relation, target) {
        return String(relation.column)
                + " -> "
                + String(relation.referenceColumn)
    }

    function drawRelationLabel(context, geometry, label) {
        context.font = "12px sans-serif"
        const measuredWidth = context.measureText(label).width
        const width = Math.min(330, measuredWidth + 16)
        const x = geometry.labelX - width / 2
        const y = geometry.labelY - 11

        context.fillStyle = "#ffffff"
        context.strokeStyle = "#cbd5e1"
        context.lineWidth = 1
        context.fillRect(x, y, width, 22)
        context.strokeRect(x, y, width, 22)
        context.fillStyle = "#344054"
        context.textAlign = "center"
        context.textBaseline = "middle"
        context.fillText(label,
                         geometry.labelX,
                         geometry.labelY,
                         width - 10)
    }

    function paintRelations(context) {
        context.reset()

        for (let source = 0; source < tableCount; ++source) {
            const relations = schema[source].relations || []
            for (let relationIndex = 0;
                 relationIndex < relations.length;
                 ++relationIndex) {
                const relation = relations[relationIndex]
                const target = tableIndex(relation.referenceTable)
                if (target < 0)
                    continue

                const lane = (relationIndex
                              - (relations.length - 1) / 2) * 18
                const geometry = relationGeometry(
                                   source,
                                   target,
                                   lane)
                context.save()
                context.lineWidth = relation.identifying ? 2.5 : 2
                context.strokeStyle = relation.identifying
                                      ? "#175cd3"
                                      : "#667085"
                if (context.setLineDash)
                    context.setLineDash(
                                relation.identifying ? [] : [7, 5])

                context.beginPath()
                context.moveTo(geometry.points[0].x,
                               geometry.points[0].y)
                for (let pointIndex = 1;
                     pointIndex < geometry.points.length;
                     ++pointIndex)
                    context.lineTo(geometry.points[pointIndex].x,
                                   geometry.points[pointIndex].y)
                context.stroke()

                if (context.setLineDash)
                    context.setLineDash([])
                drawCardinality(
                            context,
                            geometry.points[0].x,
                            geometry.points[0].y,
                            geometry.sourceDx,
                            geometry.sourceDy,
                            relation.sourceCardinality)
                const lastPoint =
                    geometry.points[geometry.points.length - 1]
                drawCardinality(
                            context,
                            lastPoint.x,
                            lastPoint.y,
                            geometry.targetDx,
                            geometry.targetDy,
                            relation.targetCardinality)
                context.restore()

                drawRelationLabel(
                            context,
                            geometry,
                            compactRelationLabel(
                                source,
                                relation,
                                target))
            }
        }
    }

    onSchemaChanged: relationCanvas.requestPaint()
    onOrderedIndicesChanged: relationCanvas.requestPaint()
    onColumnCountChanged: relationCanvas.requestPaint()
    onSurfaceHeightChanged: relationCanvas.requestPaint()

    Rectangle {
        anchors.fill: parent
        color: "#f8fafc"
        border.color: "#d0d5dd"
    }

    Label {
        anchors.centerIn: parent
        visible: root.tableCount === 0
        text: "No entity-relationship model is available."
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
            width: root.surfaceWidth
            height: root.surfaceHeight

            Canvas {
                id: relationCanvas
                anchors.fill: parent
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()
                onPaint: root.paintRelations(getContext("2d"))
            }

            Repeater {
                model: root.schema || []

                delegate: SchemaTableNode {
                    required property int index
                    required property var modelData

                    objectName: "erEntity_" + String(modelData.name)
                    x: root.nodeX(index)
                    y: root.nodeY(index)
                    width: root.nodeWidth
                    height: root.nodeHeight(index)
                    tableData: modelData
                }
            }
        }
    }
}
