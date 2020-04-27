import QtQuick 2.14
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.14

ListView {
    width:  parent.width
    height: parent.height

    keyNavigationEnabled: true
    activeFocusOnTab: true

    highlightMoveVelocity : 1000000
    highlight: Component { Rectangle {
        color: "lightblue"
        width: parent.width
    } }

    ScrollBar.vertical: ScrollBar {}
}