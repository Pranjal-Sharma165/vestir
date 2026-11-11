import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property CalendarPageModel calendarPageModel

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: Theme.spacing.pageSides
        spacing: 12

        Label {
            text: qsTr("Settings")
            font: Theme.font.title
        }

        Label {
            text: qsTr("Calendar")
            font: Theme.font.subtitle
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font: Theme.font.body
            text: root.calendarPageModel.connected
                  ? qsTr("Calendar connected.")
                  : qsTr("Paste the secret iCal address from Google Calendar so outfits can match your events.")
        }

        TextField {
            id: linkField
            Layout.fillWidth: true
            visible: !root.calendarPageModel.connected
            placeholderText: qsTr("iCal link (https:// or webcal://)")
            font: Theme.font.body
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.WordWrap
            font: Theme.font.caption
            color: "#B00020"
            text: root.calendarPageModel.status
        }

        RowLayout {
            spacing: 8

            StyledButton {
                visible: !root.calendarPageModel.connected
                enabled: linkField.text.trim().length > 0 && !root.calendarPageModel.busy
                text: root.calendarPageModel.busy ? qsTr("Connecting...") : qsTr("Connect")
                onClicked: {
                    root.calendarPageModel.saveLink(linkField.text)
                    linkField.text = ""
                }
            }

            StyledButton {
                visible: root.calendarPageModel.connected
                enabled: !root.calendarPageModel.busy
                text: root.calendarPageModel.busy ? qsTr("Refreshing...") : qsTr("Refresh")
                onClicked: root.calendarPageModel.refresh()
            }

            StyledButton {
                visible: root.calendarPageModel.connected
                enabled: !root.calendarPageModel.busy
                text: qsTr("Disconnect")
                onClicked: root.calendarPageModel.disconnectCalendar()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
