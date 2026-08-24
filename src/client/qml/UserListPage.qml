import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: userPage

    required property var apiClient
    required property var sessionState

    property int listReqId: 0
    property var userModel: []

    Component.onCompleted: loadUsers()

    function loadUsers() {
        listReqId = 2000 + Math.floor(Math.random() * 10000)
        apiClient.sendRequest("USER_LIST", listReqId, {"page": 1, "page_size": 50})
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr("Users"); font.bold: true; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); onClicked: loadUsers() }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: userModel
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
                    Label { text: modelData.username || ""; font.bold: true; width: 120 }
                    Label { text: modelData.full_name || ""; Layout.fillWidth: true }
                    Label { text: modelData.role || ""; color: "#666"; width: 80 }
                    Label { text: modelData.department || ""; color: "#999"; width: 100 }
                    Label { text: modelData.is_active ? qsTr("active") : qsTr("inactive"); color: modelData.is_active ? "green" : "red" }
                }
            }

            BusyIndicator { anchors.centerIn: parent; running: apiClient.busy }
        }

        Label { text: qsTr("Total: ") + (userModel.length || 0); color: "#666" }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId !== listReqId) return
        if (response.status === "ok" && response.data)
            userModel = response.data.items || []
    }
}
