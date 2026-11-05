pragma Singleton

import QtQuick

QtObject {
    id: vestir_Semantic_colours

    property Vestir_Semantic_coloursTheme activeTheme: _default
    property Vestir_Semantic_coloursColors colors: activeTheme.colors

    property Vestir_Semantic_coloursTheme _default: Vestir_Semantic_coloursTheme {
        colors: Vestir_Semantic_coloursColors {
            action_primary: Vestir_Foundations.colors.colour_alt_c1
            background_camera: Vestir_Foundations.colors.colour_neutral_c7
            background_default: Vestir_Foundations.colors.colour_neutral_c1
            border_accent: Vestir_Foundations.colors.colour_alt_c1
            border_inverse: Vestir_Foundations.colors.colour_neutral_c4
            border_subtle: Vestir_Foundations.colors.colour_neutral_c5
            garment_line: Vestir_Foundations.colors.colour_neutral_c5
            icon_inverse: Vestir_Foundations.colors.colour_neutral_c4
            icon_primary: Vestir_Foundations.colors.colour_neutral_c2
            icon_secondary: Vestir_Foundations.colors.colour_neutral_c3
            icon_success: Vestir_Foundations.colors.colour_alt_c2
            placeholder_image_1: "#2b9e79"
            placeholder_image_2: "#2b6e9e"
            surface_accent: Vestir_Foundations.colors.colour_alt_c3
            surface_default: Vestir_Foundations.colors.colour_neutral_c4
            surface_inverse: Vestir_Foundations.colors.colour_neutral_c2
            surface_subtle: Vestir_Foundations.colors.colour_neutral_c6
            text_accent: Vestir_Foundations.colors.colour_alt_c1
            text_inverse: Vestir_Foundations.colors.colour_neutral_c4
            text_inverse_muted: Vestir_Foundations.colors.colour_neutral_c6
            text_primary: Vestir_Foundations.colors.colour_neutral_c2
            text_secondary: Vestir_Foundations.colors.colour_neutral_c3
            text_success: Vestir_Foundations.colors.colour_alt_c2
        }
    }
}