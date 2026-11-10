import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Vestir

ApplicationWindow {
    required property Closet closet
    required property RecommendationPageModel recommendationPageModel
    required property CalendarPageModel calendarPageModel
    id: window
    width: 390
    height: 800
    visible: true
    flags: Qt.Window | Qt.ExpandedClientAreaHint

    palette {
        window: Theme.colours.background_c1
        text: Theme.colours.text
        windowText: Theme.colours.text
        highlight: Theme.colours.text_accent
    }

    readonly property var pageTitles: [
        qsTr("Home"),
        qsTr("Outfits"),
        // qsTr("Closet"),
        qsTr("Calendar"),
        qsTr("History"),
        qsTr("Settings"),
    ]

    Item {
        anchors.fill: parent
        StackView {
            id: mainStack
            anchors.fill: parent
            initialItem:  ColumnLayout {
                spacing: 0
                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    StackLayout {
                        id: mainView
                        anchors.fill: parent
                        currentIndex: taskbar.currentIndex

                        // Home - Clothing Inventory
                        ClosetPage {
                            closet: window.closet
                            stack: mainStack
                        }

                        // Outfits - AI recommendation
                        HomePage {
                            recommendationPageModel: window.recommendationPageModel
                        }

                        CalendarPage {
                            calendarPageModel: window.calendarPageModel
                            stack: mainStack
                        }
                        HistoryPage { }
                        SettingsPage { }
                    }
                }

                TabBar {
                    Layout.fillWidth: true
                    id: taskbar
                    currentIndex: mainView.currentIndex

                    TabButton {
                        text: qsTr("Home")
                    }
                    TabButton {
                        text: qsTr("Outfits")
                    }
                    TabButton {
                        text: qsTr("Calendar")
                    }
                    TabButton {
                        text: qsTr("History")
                    }
                    TabButton {
                        text: qsTr("Settings")
                    }
                }
            }
        }
    }
}
