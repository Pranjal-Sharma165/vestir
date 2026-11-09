import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Button {
    id: root
    required property int dateOffset

    function getOffsetDate(offset) {
        let d = new Date();
        d.setDate(d.getDate() + offset);
        return d;
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: Qt.formatDate(root.getOffsetDate(root.dateOffset), "ddd")
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.getOffsetDate(root.dateOffset).getDate()
        }
    }
}
