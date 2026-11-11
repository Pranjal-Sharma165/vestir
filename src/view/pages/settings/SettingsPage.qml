import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property CalendarPageModel calendarPageModel
    required property WeatherSettingsModel weatherSettingsModel
    required property RecommendationPageModel recommendationPageModel

    // White rounded card like the calendar event cards
    component Card: Rectangle {
        default property alias content: column.data
        Layout.fillWidth: true
        implicitHeight: column.implicitHeight + 32
        radius: 22
        color: "#FFFFFF"

        ColumnLayout {
            id: column
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 16
            }
            spacing: 10
        }
    }

    component CardField: TextField {
        Layout.fillWidth: true
        font: Theme.font.body
        leftPadding: 14
        background: Rectangle {
            radius: 22
            color: Theme.colours.background_c2
            border.width: 1
            border.color: Theme.colours.background_c1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: Theme.spacing.pageSides
        spacing: 16

        Label {
            text: qsTr("Settings")
            font: Theme.font.title
            color: Theme.colours.text
            Layout.bottomMargin: 8
        }

        Card {
            Label {
                text: qsTr("Calendar")
                font: Theme.font.bodyBold
                color: Theme.colours.text
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font: Theme.font.caption
                color: Theme.colours.alt_c2
                text: root.calendarPageModel.connected
                      ? qsTr("Calendar connected.")
                      : qsTr("Paste the secret iCal address from Google Calendar so outfits can match your events.")
            }

            CardField {
                id: linkField
                visible: !root.calendarPageModel.connected
                placeholderText: qsTr("iCal link (https:// or webcal://)")
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

                StyledButtonDark {
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
                    text: qsTr("Disconnect")
                    onClicked: root.calendarPageModel.disconnectCalendar()
                }
            }
        }

        Card {
            Label {
                text: qsTr("Weather")
                font: Theme.font.bodyBold
                color: Theme.colours.text
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font: Theme.font.caption
                color: Theme.colours.alt_c2
                text: qsTr("Weather city: %1").arg(root.weatherSettingsModel.city)
            }

            CardField {
                id: cityField
                placeholderText: qsTr("Search for a city")
                inputMethodHints: Qt.ImhNoPredictiveText
                onTextEdited: searchTimer.restart()
                Timer {
                    id: searchTimer
                    interval: 400
                    onTriggered: root.weatherSettingsModel.search(cityField.text)
                }
            }

            BusyIndicator {
                visible: root.weatherSettingsModel.busy
                running: visible
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                wrapMode: Text.WordWrap
                font: Theme.font.caption
                color: "#B00020"
                text: root.weatherSettingsModel.status
            }

            Repeater {
                model: root.weatherSettingsModel.results
                delegate: StyledButton {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    onClicked: {
                        // Clear first: choose() removes this button from the list
                        searchTimer.stop()
                        cityField.clear()
                        cityField.focus = false
                        Qt.inputMethod.reset()
                        root.weatherSettingsModel.choose(index)
                    }
                }
            }
        }

        StyledButtonDark {
            Layout.alignment: Qt.AlignHCenter
            enabled: !root.calendarPageModel.busy
            text: root.calendarPageModel.busy ? qsTr("Refreshing...")
                                              : qsTr("Refresh calendar & weather")
            onClicked: {
                if (root.calendarPageModel.connected)
                    root.calendarPageModel.refresh()
                root.recommendationPageModel.refreshWeather()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
