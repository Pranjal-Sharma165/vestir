import QtQuick
import QtQuick.Controls
import Vestir

Button {
    id: root
    required property int dateOffset
    property bool selected: false

    function getOffsetDate(offset) {
        let d = new Date();
        d.setDate(d.getDate() + offset);
        d.setHours(0, 0, 0, 0);
        return d;
    }

    readonly property date day: getOffsetDate(dateOffset)

    implicitWidth: 56
    implicitHeight: 76
    padding: 0

    background: Rectangle {
        radius: 16
        color: root.selected ? Theme.colours.alt_c1
                             : (root.pressed ? Theme.colours.background_c2_pressed
                                             : Theme.colours.background_c2)
    }

    contentItem: Item {
        Column {
            anchors.centerIn: parent
            spacing: 6

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.formatDate(root.day, "ddd")
                font: Theme.font.body
                color: root.selected ? Theme.colours.text_light : Theme.colours.text
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.day.getDate()
                font: Theme.font.subtitle
                color: root.selected ? Theme.colours.text_light : Theme.colours.text
            }
        }
    }
}
