import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: mainPage
    background: null

    required property var client
    required property var session

    property int nextReqId: 100

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
        dispatchResponse(reqId, response)
    }

    function dispatchResponse(reqId, response) {
        var item = stackLayout.children[stackLayout.currentIndex]
        if (item && item.handleResponse) {
            item.handleResponse(reqId, response)
        }
    }
}
