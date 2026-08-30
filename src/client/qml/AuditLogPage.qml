import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: auditPage
    background: null

    required property var client
    required property var session

    property int queryReqId: 0
    property var auditModel: []

    Component.onCompleted: loadAudit()

    function loadAudit() {
        queryReqId = 4000 + Math.floor(Math.random() * 10000)
        client.sendRequest("AUDIT_QUERY", queryReqId, {"page": 1, "page_size": 100})
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr("Audit Log"); font.bold: true; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); font.pointSize: 12; onClicked: loadAudit() }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: auditModel
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: 45
                border.color: "#ddd"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { text: modelData.ts || ""; color: "#666"; width: 160 }
                    Label { text: modelData.actor_name || ""; width: 100 }
                    Label { text: modelData.action || ""; font.bold: true; width: 140 }
                    Label { text: modelData.target_type || ""; color: "#999"; width: 80 }
                    Label { text: modelData.result || ""; color: modelData.result === "ok" ? "green" : "red" }
                }
            }

            BusyIndicator { anchors.centerIn: parent; running: client.busy }
        }

        Label { text: qsTr("Total: ") + (auditModel.length || 0); color: "#666" }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId !== queryReqId) return
        if (response.status === "ok" && response.data)
            auditModel = response.data.items || []
    }
}
