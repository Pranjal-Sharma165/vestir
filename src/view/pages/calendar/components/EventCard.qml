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

    readonly property string dayLabel: isToday
        ? qsTr("Today")
        : Qt.formatDate(calendarEvent.startTime, "ddd").toUpperCase()

    readonly property string timeRange: calendarEvent.allDay
        ? qsTr("All day")
        : Qt.formatTime(calendarEvent.startTime, "hh:mm") + "-"
          + Qt.formatTime(calendarEvent.endTime, "hh:mm")

    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent
        RowLayout {
            // The date
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillHeight: true
                Layout.preferredWidth: 60
                spacing: 2
                Label {
                    text: Qt.formatTime(calendarEvent.startTime, "hh:mm")
                    font: Theme.font.caption
                }
                Label {
                    text: dayLabel
                    font: Theme.font.caption
                }
            }

            // Event information
            ColumnLayout {
                Layout.fillWidth: true

                Label {
                    text: calendarEvent.qName
                    font: Theme.font.body
                }

                Label {
                    text: timeRange + "  |  " + calendarEvent.location
                    font: Theme.font.body
                }

            }
        }
        RowLayout {
            Layout.fillWidth: true

            ComboBox {
                id: styleBox
                model: OutfitFormality.qGetList()
                displayText: OutfitFormality.qDisplayName(currentValue)
                delegate: ItemDelegate {
                    required property var modelData
                    width: styleBox.width
                    text: OutfitFormality.qDisplayName(modelData)
                    highlighted: styleBox.highlightedIndex === index
                }
                currentIndex: model.indexOf(root.calendarEvent.qFormality)
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "see current outfit"
                font: Theme.font.body
            }
        }
    }
}
