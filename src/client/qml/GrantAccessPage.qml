import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: grantPage
    background: null

    required property var client
    required property var session

    property int usersReqId: 0
    property int resourcesReqId: 0
    property int policiesReqId: 0
    property int grantReqId: 0
    property int revokeReqId: 0
    property var userModel: []
    property var resourceModel: []

    ListModel { id: grantModel }

    Component.onCompleted: {
        loadUsers()
        loadResources()
        loadPolicies()
    }

    function loadUsers() {
        usersReqId = 8000 + Math.floor(Math.random() * 10000)
        client.sendRequest("USER_LIST", usersReqId, {"page": 1, "page_size": 100})
    }

    function loadResources() {
        resourcesReqId = 8100 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_LIST", resourcesReqId, {"page": 1, "page_size": 100})
    }

    function loadPolicies() {
        policiesReqId = 8200 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_LIST", policiesReqId, {"page": 1, "page_size": 100})
    }

    function grantAccess() {
        var ui = userCombo.currentIndex
        var ri = resourceCombo.currentIndex
        if (ui < 0 || ri < 0 || userModel.length === 0 || resourceModel.length === 0)
            return
        grantReqId = 8300 + Math.floor(Math.random() * 10000)
        client.sendRequest("GRANT_ACCESS", grantReqId, {
            "subject_id": userModel[ui].id,
            "resource_id": resourceModel[ri].id,
            "action": grantActionCombo.currentText
        })
    }

    function revokePolicy(policyId) {
        revokeReqId = 8400 + Math.floor(Math.random() * 10000)
        client.sendRequest("REVOKE_ACCESS", revokeReqId, {"policy_id": policyId})
    }

    function refreshGrantPolicies(policies) {
        grantModel.clear()
        for (var i = 0; i < policies.length; i++) {
            var p = policies[i]
            if (p.name && String(p.name).indexOf("grant-") === 0)
                grantModel.append({id: p.id, name: p.name, action: p.action || ""})
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: qsTr("Grant / Revoke Access")
            font.bold: true
            font.pixelSize: 18
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Grant Access")

            ColumnLayout {
                width: parent.width
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ComboBox {
                        id: userCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        model: userModel
                        textRole: "username"
                        enabled: userModel.length > 0
                    }
                    ComboBox {
                        id: resourceCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        model: resourceModel
                        textRole: "name"
                        enabled: resourceModel.length > 0
                    }
                    ComboBox {
                        id: grantActionCombo
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 30
                        model: ["read", "write", "manage", "connect", "select", "modify", "print", "access", "admin"]
                    }
                    Button {
                        text: qsTr("Grant")
                        font.pointSize: 12
                        onClicked: grantAccess()
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("Grants (revoke)")

            ListView {
                anchors.fill: parent
                anchors.margins: 4
                model: grantModel
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 40
                    border.color: "#ddd"
                    border.width: 1
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Label { text: model.name || ""; font.bold: true; Layout.fillWidth: true }
                        Label { text: model.action || ""; color: "#666"; width: 80 }
                        Button {
                            text: qsTr("Revoke")
                            font.pointSize: 10
                            onClicked: revokePolicy(model.id)
                        }
                    }
                }
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: client.busy
            visible: client.busy
        }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId === usersReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            if (response.data)
                userModel = response.data.items || []
            return
        }
        if (reqId === resourcesReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            if (response.data)
                resourceModel = response.data.items || []
            return
        }
        if (reqId === policiesReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            if (response.data)
                refreshGrantPolicies(response.data.items || [])
            return
        }
        if (reqId === grantReqId || reqId === revokeReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            loadPolicies()
            return
        }
    }
}
