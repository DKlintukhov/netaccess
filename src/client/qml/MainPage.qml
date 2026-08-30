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
                text: "NetAccess"
                font.pointSize: 13
                font.bold: true
                padding: 8
            }
            Item { Layout.fillWidth: true }
            Label {
                text: sessionState.username + " (" + sessionState.role + ")"
                font.pointSize: 12
                padding: 8
            }
            Button {
                text: qsTr("Logout")
                font.pointSize: 12
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
        font.pointSize: 12
        width: parent.width
        currentIndex: stackLayout.currentIndex

        TabButton { text: qsTr("Resources") }
        TabButton {
            text: qsTr("Users")
            visible: sessionState.isAdmin()
        }
        TabButton {
            text: qsTr("Policies")
            visible: sessionState.isAdmin() || sessionState.role === "auditor"
        }
        TabButton {
            text: qsTr("Audit Log")
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
            apiClient: mainPage.apiClient
            sessionState: mainPage.sessionState
        }

        UserListPage {
            apiClient: mainPage.apiClient
            sessionState: mainPage.sessionState
        }

        PolicyListPage {
            apiClient: mainPage.apiClient
            sessionState: mainPage.sessionState
        }

        AuditLogPage {
            apiClient: mainPage.apiClient
            sessionState: mainPage.sessionState
        }
    }

    signal handleResponse(int reqId, var response)

    function dispatchResponse(reqId, response) {
        var item = stackLayout.currentItem
        if (item && item.handleResponse) {
            item.handleResponse(reqId, response)
        }
    }
}
