import QtQuick

Window {
    width: 320
    height: 200
    visible: true
    title: "netaccess client"

    Text {
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        text: "Hello from netaccess_client\nversion " + netaccessVersion
    }
}
