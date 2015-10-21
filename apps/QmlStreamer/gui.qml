import QtQuick 2.4
import QtQuick.Window 2.2

Item
{
    objectName: "QmlStreamer example"

    onWindowChanged: {
        // tell the window/streamer our size constrains
        window.width = 300
        window.height = 300
        window.minimumWidth = 300
        window.minimumHeight = 300
        window.maximumWidth = 500
        window.maximumHeight = 500
    }

    width: 300
    height: 300
    Column
    {
        spacing: 2

        Rectangle { color: "red"; width: 50; height: 50 }
        Rectangle { id: greenRect; color: "green"; width: 20; height: 50 }
        Rectangle { color: "blue"; width: 50; height: 20 }

        move: Transition {
            NumberAnimation { properties: "x,y"; duration: 1000 }
        }

        focus: true
    }

    MouseArea
    {
        anchors.fill: parent
        onClicked: greenRect.visible = !greenRect.visible
    }
}
