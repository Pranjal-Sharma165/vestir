import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Vestir

TabButton {
    id: root

    property string iconName: "home"
    property string label: ""
    readonly property color activeColor: Theme.colours.alt_c1
    readonly property color inactiveColor: "#8293A3"

    implicitHeight: 64
    padding: 0

    background: Item {}   // no default tab background

    contentItem: ColumnLayout {
        spacing: 3

        Item {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 56
            implicitHeight: 30

            // pill highlight behind the active icon
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: Theme.colours.alt_c3
                opacity: root.checked ? 0.55 : 0
                scale: root.checked ? 1 : 0.6
                Behavior on opacity { NumberAnimation { duration: 180 } }
                Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
            }

            NavigationIcon {
                anchors.centerIn: parent
                iconName: root.iconName
                iconColor: root.checked ? root.activeColor : root.inactiveColor
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.label
            font: Qt.font({
                family: Theme.fontFamily.body,
                pixelSize: 11,
                bold: root.checked
            })
            color: root.checked ? root.activeColor : root.inactiveColor
        }
    }
}
