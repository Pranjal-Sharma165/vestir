import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property CalendarPageModel calendarPageModel
    required property StackView stack
    signal showOutfits(var eventId)

    // Event types are filled in once outfits were requested, so re-read.
    onVisibleChanged: if (visible) calendarPageModel.reload()
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        spacing: 0

        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 84
            contentWidth: datesRow.implicitWidth
            contentHeight: height
            clip: true
            flickableDirection: Flickable.HorizontalFlick

            RowLayout {
                id: datesRow
                height: parent.height
                spacing: 4

                Item {
                    width: Theme.spacing.pageSides - datesRow.spacing
                }
                Repeater {
                    // show maximum of 2 months' dates at a time
                    model: 28
                    delegate: DayOfWeekButton {
                        required property int index
                        Layout.preferredWidth: 56
                        Layout.fillHeight: true
                        dateOffset: index
                        selected: calendarPageModel.selectedDate.getUTCFullYear() === day.getFullYear()
                                  && calendarPageModel.selectedDate.getUTCMonth() === day.getMonth()
                                  && calendarPageModel.selectedDate.getUTCDate() === day.getDate()
                        onClicked: calendarPageModel.selectedDate = day
                    }
                }
                Item {
                    width: Theme.spacing.pageSides - datesRow.spacing
                }
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.spacing.pageSides
            Layout.rightMargin: Layout.leftMargin
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentWidth: availableWidth
            ColumnLayout {
                width: scrollView.availableWidth
                Label {
                    visible: calendarPageModel.items.length === 0
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font: Theme.font.body
                    text: calendarPageModel.connected
                          ? qsTr("No events on this day")
                          : qsTr("No calendar connected. Add your iCal link in Settings.")
                }
                Repeater {
                    model: calendarPageModel.items
                    delegate: EventCard {
                        required property CalendarEvent modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        calendarEvent: modelData
                        onOutfitRequested: root.showOutfits(modelData.eventId)
                    }
                }
                Item {
                    height: Theme.spacing.pageBottom
                }
            }
        }
    }
}
