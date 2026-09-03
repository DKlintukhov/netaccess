import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: mainPage
    background: null

    required property var client
    required property var session

    property int nextReqId: 100
    property int meReqId: 0

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: "NetAccess"
                font.pointSize: 13
                font.bold: true
                padding: 8
            }
            Item { Layout.fillWidth: true }
            Label {
                text: session.username + " (" + session.role + ")"
                font.pointSize: 12
                padding: 8
            }
            Button {
                text: qsTr("Profile")
                font.pointSize: 12
                onClicked: {
                    meReqId = 9500 + Math.floor(Math.random() * 10000)
                    client.sendRequest("ME", meReqId, {})
                    profileDialog.open()
                }
            }
            Button {
                text: qsTr("Logout")
                font.pointSize: 12
                onClicked: {
                    client.sendRequest("LOGOUT", nextReqId++)
                    session.clear()
                }
            }
        }
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        font.pointSize: 12
        width: parent.width
        currentIndex: stackLayout.currentIndex

        TabButton { text: qsTr("Resources") }
        TabButton { text: qsTr("Access Check") }
        TabButton {
            text: qsTr("Users")
            visible: session.isAdmin()
        }
        TabButton {
            text: qsTr("Grant Access")
            visible: session.isAdmin()
        }
        TabButton {
            text: qsTr("Policies")
            visible: session.isAdmin() || session.role === "auditor"
        }
        TabButton {
            text: qsTr("Audit Log")
            visible: session.isAdmin() || session.role === "auditor"
        }
    }

    StackLayout {
        id: stackLayout
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: tabBar.currentIndex

        ResourceListPage {
            id: resourcePage
            client: mainPage.client
            session: mainPage.session
        }

        AccessCheckPage {
            id: accessPage
            client: mainPage.client
            session: mainPage.session
        }

        UserListPage {
            id: userPage
            client: mainPage.client
            session: mainPage.session
        }

        GrantAccessPage {
            id: grantPage
            client: mainPage.client
            session: mainPage.session
        }

        PolicyListPage {
            id: policyPage
            client: mainPage.client
            session: mainPage.session
        }

        AuditLogPage {
            id: auditPage
            client: mainPage.client
            session: mainPage.session
        }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId === meReqId) {
            if (response.status !== "ok") {
                errorLabel.text = response.message || response.code || qsTr("Error")
                errorDialog.open()
                return
            }
            var d = response.data || {}
            profUsername.text = d.username || ""
            profName.text = d.full_name || ""
            profRole.text = d.role || ""
            profDept.text = d.department || ""
            profClearance.text = String(d.clearance_level !== undefined ? d.clearance_level : "")
            profPosition.text = d.position || ""
            return
        }
        dispatchResponse(reqId, response)
    }

    function dispatchResponse(reqId, response) {
        for (var i = 0; i < stackLayout.children.length; i++) {
            var item = stackLayout.children[i]
            if (item && item.handleResponse) {
                item.handleResponse(reqId, response)
            }
        }
    }

    Dialog {
        id: profileDialog
        title: qsTr("My Profile")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close
        width: 360

        GridLayout {
            width: parent.width
            columns: 2
            columnSpacing: 12
            rowSpacing: 6
            Label { text: qsTr("Username:"); color: "#666" }
            Label { id: profUsername; text: ""; font.bold: true }
            Label { text: qsTr("Full name:"); color: "#666" }
            Label { id: profName; text: "" }
            Label { text: qsTr("Role:"); color: "#666" }
            Label { id: profRole; text: "" }
            Label { text: qsTr("Department:"); color: "#666" }
            Label { id: profDept; text: "" }
            Label { text: qsTr("Clearance level:"); color: "#666" }
            Label { id: profClearance; text: "" }
            Label { text: qsTr("Position:"); color: "#666" }
            Label { id: profPosition; text: "" }
        }
    }
}
