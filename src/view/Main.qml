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

    palette {
        window: Theme.colours.background_c1
        text: Theme.colours.text
        windowText: Theme.colours.text
        highlight: Theme.colours.text_accent
    }

    readonly property var pageTitles: [
        qsTr("Home"),
        qsTr("Closet"),
        qsTr("Calendar"),
        qsTr("History"),
        qsTr("Settings"),
    ]

    Pane {
        anchors.fill: parent
        padding: 0
        StackView {
            id: mainStack
            anchors.fill: parent
            initialItem:  ColumnLayout {
                spacing: 0
                Pane {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    verticalPadding: 0

                    StackLayout {
                        id: mainView
                        anchors.fill: parent
                        currentIndex: taskbar.currentIndex

                        HomePage {
                            recommendationPageModel: window.recommendationPageModel
                        }
                        ClosetPage {
                            closet: window.closet
                            stack: mainStack
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
                        text: qsTr("Closet")
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
