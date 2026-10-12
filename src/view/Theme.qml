pragma Singleton
import QtQuick

QtObject {
    readonly property string fontFamily: "Inter"

    readonly property font title: Qt.font({
        family: fontFamily,
        pixelSize: 28,
    })
    readonly property font body: Qt.font({
        family: fontFamily,
        pixelSize: 13
    })
    readonly property font caption: Qt.font({
        family: fontFamily,
        pixelSize: 11
    })
}
