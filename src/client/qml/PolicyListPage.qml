import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: policyPage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property var policyModel: []

    Component.onCompleted: loadPolicies()

    function loadPolicies() {
        listReqId = 3000 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_LIST", listReqId, {"page": 1, "page_size": 50})
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr("Policies"); font.bold: true; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            Button { text: "Refresh"; font.pointSize: 12; onClicked: loadPolicies() }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: policyModel
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: 50
                border.color: "#ddd"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { text: modelData.name || ""; font.bold: true; width: 180 }
                    Label { text: modelData.action || ""; color: "#666"; width: 80 }
                    Label { text: modelData.role_required || "*"; color: "#999"; width: 80 }
                    Label { text: modelData.resource_type || "*"; color: "#999"; width: 100 }
                    Label { text: "P:" + (modelData.priority || 0); width: 50 }
                    Label { text: modelData.enabled ? qsTr("ON") : qsTr("OFF"); color: modelData.enabled ? "green" : "red" }
                }
            }

            BusyIndicator { anchors.centerIn: parent; running: client.busy }
        }

        Label { text: qsTr("Total: ") + (policyModel.length || 0); color: "#666" }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId !== listReqId) return
        if (response.status === "ok" && response.data)
            policyModel = response.data.items || []
    }
}
