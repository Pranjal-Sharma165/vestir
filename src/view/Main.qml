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
                            onShowOutfits: (eventId) => {
                                window.recommendationPageModel.showEvent(eventId)
                                taskbar.currentIndex = 1
                            }
                        }
                        HistoryPage { }
                        SettingsPage {
                            calendarPageModel: window.calendarPageModel
                        }
                    }
                }



                Item {
                    Layout.fillWidth: true
                    implicitHeight: taskbar.implicitHeight + 24

                    TabBar {
                        id: taskbar
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        anchors.bottomMargin: 14
                        anchors.topMargin: 6
                        currentIndex: mainView.currentIndex
                        padding: 0

                        background: Rectangle {
                            color: "#FFFFFF"
                            radius: 26
                            border.width: 1
                            border.color: Theme.colours.background_c1
                        }

                        NavTabButton { iconName: "home";     label: qsTr("Home") }
                        NavTabButton { iconName: "outfits";  label: qsTr("Outfits") }
                        NavTabButton { iconName: "calendar"; label: qsTr("Calendar") }
                        NavTabButton { iconName: "history";  label: qsTr("History") }
                        NavTabButton { iconName: "settings"; label: qsTr("Settings") }
                    }
                }
            }
        }
    }
}
