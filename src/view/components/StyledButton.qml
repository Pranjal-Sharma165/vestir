import QtQuick
import QtQuick.Controls.Basic
import Vestir

Button {
    id: button
    font: Theme.font.button
    horizontalPadding: 15

    property color textNormal: Theme.colours.text
    property color textPressed: Theme.colours.text
    property color backgroundNormal: Theme.colours.background_c2
    property color backgroundPressed: Theme.colours.background_c2_pressed
    property color borderNormal: Theme.colours.background_c1
    property color borderPressed: Theme.colours.background_c1_pressed

    property color checkedTextNormal: Theme.colours.text_light
    property color checkedTextPressed: Theme.colours.text_light
    property color checkedBackgroundNormal: Theme.colours.alt_c1
    property color checkedBackgroundPressed: Theme.colours.alt_c1_pressed
    property color checkedBorderNormal: checkedBackgroundNormal
    property color checkedBorderPressed: checkedBackgroundPressed

    contentItem: Text {
        text: button.text
        font: button.font
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        opacity: enabled ? 1.0 : 0.3
        color: (button.checkable && button.checked) ?
                   (button.pressed ? checkedTextPressed : checkedTextNormal) :
                   (button.pressed ? textPressed : textNormal)
    }

    background: Rectangle {
        radius: 17
        implicitHeight: 34
        opacity: enabled ? 1 : 0.3
        border.color: (button.checkable && button.checked) ?
                   (button.pressed ? checkedBorderPressed : checkedBorderNormal) :
                   (button.pressed ? borderPressed : borderNormal)
        border.width: 1.5
        color: (button.checkable && button.checked) ?
                   (button.pressed ? checkedBackgroundPressed : checkedBackgroundNormal) :
                   (button.pressed ? backgroundPressed : backgroundNormal)
    }
}
