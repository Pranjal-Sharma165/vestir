import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property CalendarPageModel calendarPageModel
    required property StackView stack
    ColumnLayout {
        anchors.fill: parent

        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: datesRow.implicitHeight
            contentWidth: datesRow.implicitWidth
            contentHeight: height
            clip: true
            flickableDirection: Flickable.HorizontalFlick

            RowLayout {
                id: datesRow
                height: parent.height
                spacing: 4

                Repeater {
                    // show maximum of 2 months' dates at a time
                    model: 28
                    delegate: DayOfWeekButton {
                        required property int index
                        Layout.fillHeight: true
                        dateOffset: index
                    }
                }
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentWidth: availableWidth
            ColumnLayout {
                width: scrollView.availableWidth
                Repeater {
                    model: calendarPageModel.items
                    delegate: EventCard {
                        required property CalendarEvent modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        calendarEvent: modelData
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
