import QtQuick 2.4

Item
{
    objectName: "QmlStreamer example"
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
