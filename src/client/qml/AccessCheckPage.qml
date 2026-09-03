import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: accessPage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property int checkReqId: 0
    property var resourceModel: []

    Component.onCompleted: loadResources()

    function loadResources() {
        listReqId = 7000 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_LIST", listReqId, {"page": 1, "page_size": 50})
    }

    function checkAccess() {
        var idx = resourceCombo.currentIndex
        if (idx < 0 || resourceModel.length === 0) {
            resultOk.visible = false
            resultDenied.visible = false
            return
        }
        checkReqId = 7100 + Math.floor(Math.random() * 10000)
        client.sendRequest("ACCESS_CHECK", checkReqId, {
            "resource_id": resourceModel[idx].id,
            "action": actionCombo.currentText
        })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: qsTr("Access Check")
            font.bold: true
            font.pixelSize: 18
        }

        Label {
            text: qsTr("Select a resource and an action to test whether your account is permitted.")
            color: "#666"
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ComboBox {
                id: resourceCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                model: resourceModel
                textRole: "name"
                enabled: resourceModel.length > 0
            }

            ComboBox {
                id: actionCombo
                Layout.preferredWidth: 160
                Layout.preferredHeight: 32
                model: ["read", "write", "manage", "connect", "select", "modify", "print", "access", "admin"]
            }

            Button {
                text: qsTr("Check Access")
                font.pointSize: 12
                onClicked: checkAccess()
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: client.busy
            visible: client.busy
        }

        Label {
            id: resultOk
            Layout.fillWidth: true
            text: qsTr("ALLOWED")
            color: "green"
            font.bold: true
            font.pixelSize: 16
            visible: false
        }

        Label {
            id: resultDenied
            Layout.fillWidth: true
            text: ""
            color: "red"
            font.bold: true
            font.pixelSize: 16
            wrapMode: Text.Wrap
            visible: false
        }

        Item { Layout.fillHeight: true }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId === listReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            if (response.data)
                resourceModel = response.data.items || []
            return
        }
        if (reqId === checkReqId) {
            if (response.status === "ok") {
                resultOk.visible = true
                resultDenied.visible = false
            } else {
                resultOk.visible = false
                resultDenied.text = qsTr("DENIED: ") + (response.message || response.reason || response.code || qsTr("No permission"))
                resultDenied.visible = true
            }
            return
        }
    }
}
