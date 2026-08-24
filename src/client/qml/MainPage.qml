import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: mainPage

    required property var apiClient
    required property var sessionState

    property int nextReqId: 100

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: "netaccess"
                font.bold: true
                padding: 8
            }
            Item { Layout.fillWidth: true }
            Label {
                text: sessionState.username + " (" + sessionState.role + ")"
                padding: 8
            }
            Button {
                text: "Logout"
                onClicked: {
                    apiClient.sendRequest("LOGOUT", nextReqId++)
                    sessionState.clear()
                }
            }
        }
    }

    TabBar {
        id: tabBar
        anchors.top: parent.top
        width: parent.width
        currentIndex: stackLayout.currentIndex

        TabButton { text: "Resources" }
        TabButton {
            text: "Users"
            visible: sessionState.isAdmin()
        }
        TabButton {
            text: "Policies"
            visible: sessionState.isAdmin() || sessionState.role === "auditor"
        }
        TabButton {
            text: "Audit Log"
            visible: sessionState.isAdmin() || sessionState.role === "auditor"
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
            apiClient: apiClient
            sessionState: sessionState
        }

        UserListPage {
            apiClient: apiClient
            sessionState: sessionState
        }

        PolicyListPage {
            apiClient: apiClient
            sessionState: sessionState
        }

        AuditLogPage {
            apiClient: apiClient
            sessionState: sessionState
        }
    }

    signal handleResponse(int reqId, var response)

    function dispatchResponse(reqId, response) {
        // Forward to the active tab.
        var item = stackLayout.currentItem
        if (item && item.handleResponse) {
            item.handleResponse(reqId, response)
        }
    }
}
