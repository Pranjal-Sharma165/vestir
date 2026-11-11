import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

Item {
    id: root
    required property CalendarEvent calendarEvent

    signal outfitRequested()

    readonly property bool isToday:
        calendarEvent.startTime.toDateString() === new Date().toDateString()

    readonly property string timeRange: calendarEvent.allDay
        ? qsTr("All day")
        : Qt.formatTime(calendarEvent.startTime, "hh:mm") + " - "
          + Qt.formatTime(calendarEvent.endTime, "hh:mm")

    implicitHeight: card.implicitHeight + 12

    Rectangle {
        id: card
        width: parent.width
        implicitHeight: content.implicitHeight + 28
        radius: 22
        color: "#FFFFFF"

        // Coloured edge: green when an outfit is chosen
        Rectangle {
            width: 6
            radius: 3
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                margins: 14
            }
            color: root.calendarEvent.hasOutfit ? Theme.colours.alt_c1
                                                : Theme.colours.background_c1
        }

        ColumnLayout {
            id: content
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 14
                leftMargin: 28
            }
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: root.calendarEvent.qName
                font: Theme.font.bodyBold
                color: Theme.colours.text
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: root.timeRange
                      + (root.calendarEvent.location !== ""
                         ? "  ·  " + root.calendarEvent.location : "")
                font: Theme.font.caption
                color: Theme.colours.alt_c2
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    implicitHeight: 28
                    implicitWidth: styleText.implicitWidth + 24
                    radius: height / 2
                    color: Theme.colours.background_c2
                    border.width: 1
                    border.color: Theme.colours.background_c1

                    Label {
                        id: styleText
                        anchors.centerIn: parent
                        text: OutfitFormality.qDisplayName(
                                  root.calendarEvent.qFormality)
                        font: Theme.font.caption
                        color: Theme.colours.text
                    }
                }

                Label {
                    visible: root.calendarEvent.hasOutfit
                    text: qsTr("✓ Outfit chosen")
                    font: Theme.font.caption
                    color: Theme.colours.alt_c1
                }

                Item { Layout.fillWidth: true }

                StyledButtonDark {
                    visible: !root.calendarEvent.hasOutfit
                    text: qsTr("Choose outfit")
                    onClicked: root.outfitRequested()
                }
                StyledButton {
                    visible: root.calendarEvent.hasOutfit
                    text: qsTr("View outfit")
                    onClicked: root.outfitRequested()
                }
            }
        }
    }
}
