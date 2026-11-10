import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Vestir

AbstractButton {
    id: root

    property string iconName: "top"
    property string label: ""
    property int itemCount: 0
    property bool selected: false

    implicitHeight: 104
    padding: 0

    background: Rectangle {
        radius: 20
        color: root.selected ? Theme.colours.alt_c1 : "#FFFFFF"
        border.width: root.selected ? 0 : 1
        border.color: Theme.colours.background_c1
        opacity: root.pressed ? 0.85 : 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on opacity { NumberAnimation { duration: 80 } }
    }

    contentItem: ColumnLayout {
        spacing: 6

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 42
            implicitHeight: 42
            radius: width / 2
            color: root.selected ? Qt.rgba(1, 1, 1, 0.16) : Theme.colours.background_c2

            Behavior on color { ColorAnimation { duration: 150 } }

            NavigationIcon {
                anchors.centerIn: parent
                iconName: root.iconName
                iconColor: root.selected ? "#FFFFFF" : Theme.colours.alt_c1
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.label
            font: Theme.font.bodyBold
            color: root.selected ? "#FFFFFF" : Theme.colours.text
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.itemCount + (root.itemCount === 1 ? " item" : " items")
            font: Theme.font.caption
            color: root.selected ? Theme.colours.alt_c3 : Theme.colours.alt_c2
        }
    }
}
