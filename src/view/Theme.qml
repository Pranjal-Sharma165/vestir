pragma Singleton
import QtQuick

QtObject {
    property QtObject fontFamily: QtObject {
        readonly property string body: "Montserrat";
        readonly property string heading: "Outfit";
    }

    property QtObject font: QtObject {
        readonly property font title: Qt.font({
            family: Theme.fontFamily.heading,
            pixelSize: 28,
        })
        readonly property font body: Qt.font({
            family: Theme.fontFamily.body,
            pixelSize: 13
        })
        readonly property font button: Qt.font({
            family: Theme.fontFamily.body,
            pixelSize: 12
        })
        readonly property font caption: Qt.font({
            family: Theme.fontFamily.body,
            pixelSize: 11
        })
    }

    property QtObject colours: QtObject {
        readonly property string background_c1:         "#CDD6DC"
        readonly property string background_c1_pressed: "#B9C4CC"
        readonly property string background_c2:         "#E7ECEF"
        readonly property string background_c2_pressed: "#D7DEE3"
        readonly property string alt_c1:                "#274C77"
        readonly property string alt_c1_pressed:        "#1D3A5C"
        readonly property string alt_c2:                "#6096BA"
        readonly property string alt_c2_pressed:        "#4F7B98"
        readonly property string alt_c3:                "#A3CEF1"
        readonly property string alt_c3_pressed:        "#8BAFCD"
        readonly property string accent:                "#EF946C"
        readonly property string text:                  "#1B2B3F"
        readonly property string text_light:            background_c2
        readonly property string text_alt:              alt_c1
        readonly property string text_accent:           accent
    }
}