import QtQuick
import QtQuick.Controls.Basic
import Vestir

StyledButton {
    textNormal: Theme.colours.text_light
    textPressed: Theme.colours.text_light
    backgroundNormal: Theme.colours.alt_c1
    backgroundPressed: Theme.colours.alt_c1_pressed
    borderNormal: backgroundNormal
    borderPressed: backgroundPressed

    checkedTextNormal: Theme.colours.text
    checkedTextPressed: Theme.colours.text
    checkedBackgroundNormal: Theme.colours.background_c2
    checkedBackgroundPressed: Theme.colours.background_c2_pressed
    checkedBorderNormal: Theme.colours.background_c2_pressed
    checkedBorderPressed: Theme.colours.background_c2_pressed
}