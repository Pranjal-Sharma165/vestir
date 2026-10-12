import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Vestir.DesignTokens as Tokens

ApplicationWindow {
    id: window
    width: 390
    height: 800
    visible: true

    palette {
        window: Tokens.Vestir_Semantic_colours.colors.background_default
        text: Tokens.Vestir_Semantic_colours.colors.text_primary
        windowText: Tokens.Vestir_Semantic_colours.colors.text_primary
        highlight: Tokens.Vestir_Semantic_colours.colors.text_accent
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
        StackLayout {
            id: mainView
            anchors.fill: parent
            currentIndex: taskbar.currentIndex

            HomePage {}
            ClosetPage {}
            CalendarPage {}
            HistoryPage {}
            SettingsPage {}
        }
    }

    footer: TabBar {
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
