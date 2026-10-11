pragma Singleton

import QtQuick

QtObject {
    id: vestir_Foundations

    property Vestir_FoundationsTheme activeTheme: _default
    property Vestir_FoundationsColors colors: activeTheme.colors
    property Vestir_FoundationsNumbers numbers: activeTheme.numbers
    property Vestir_FoundationsStrings strings: activeTheme.strings

    property Vestir_FoundationsTheme _default: Vestir_FoundationsTheme {
        colors: Vestir_FoundationsColors {
            colour_alt_c1: "#80523e"
            colour_alt_c2: "#647362"
            colour_alt_c3: "#f0e4db"
            colour_neutral_c1: "#faf8f4"
            colour_neutral_c2: "#302b27"
            colour_neutral_c3: "#827a71"
            colour_neutral_c4: "#ffffff"
            colour_neutral_c5: "#e8e2d9"
            colour_neutral_c6: "#ede7de"
            colour_neutral_c7: "#262723"
        }
        numbers: Vestir_FoundationsNumbers {
            border_width_camera_guide: 2.5
            border_width_hairline: 1
            border_width_icon: 1.7000000476837158
            border_width_selection: 1.5
            border_width_shutter: 3
            radius_100: 100
            radius_16: 16
            radius_5: 5
            radius_8: 8
            radius_card: 16
            radius_input: 8
            radius_pill: 100
            radius_screen: 32
            radius_small_card: 12
            radius_thumbnail: 10
            size_action_button_height_52: 52
            size_icon_12: 12
            size_icon_13: 13
            size_icon_14: 14
            size_icon_15: 15
            size_icon_16: 16
            size_icon_17: 17
            size_icon_18: 18
            size_icon_20: 20
            size_icon_21: 21
            size_icon_22: 22
            size_icon_25: 25
            space_0: 0
            space_12: 12
            space_16: 16
            space_2: 2
            space_20: 20
            space_24: 24
            space_28: 28
            space_4: 4
            space_40: 40
            space_8: 8
            typography_size_10: 10
            typography_size_11: 11
            typography_size_12: 12
            typography_size_13: 13
            typography_size_14: 14
            typography_size_15: 15
            typography_size_17: 17
            typography_size_21: 21
            typography_size_22: 22
            typography_size_23: 23
            typography_size_26: 26
            typography_size_27: 27
            typography_size_28: 28
            typography_size_30: 30
            typography_size_32: 32
            typography_size_35: 35
        }
        strings: Vestir_FoundationsStrings {
            typography_family_Body: qsTr("Montserrat")
            typography_family_Headings: qsTr("Outfit")
        }
    }
}