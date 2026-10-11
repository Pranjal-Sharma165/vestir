import QtQuick
import QtQuick.Shapes
import Vestir.DesignTokens as Tokens

Shape {
    anchors.fill: parent
    id: image_placeholder
    ShapePath {
        strokeWidth: 0
        fillGradient: LinearGradient {
            x1: image_placeholder.width * 1
            x2: image_placeholder.width * -5.9604642110855366e-8
            y1: image_placeholder.height * 0
            y2: image_placeholder.height * 0.9999999542799349

            GradientStop {
                color: Tokens.Vestir_Semantic_colours.colors.placeholder_image_1
                position: 0
            }
            GradientStop {
                color: Tokens.Vestir_Semantic_colours.colors.placeholder_image_2
                position: 1
            }
        }

        PathRectangle {
            height: image_placeholder.height
            width: image_placeholder.width

            radius: 16
        }
    }
}