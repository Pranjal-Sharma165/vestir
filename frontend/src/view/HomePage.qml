import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir
import Vestir.DesignTokens as Tokens

Page {

    OutfitRecommendation { id: recommendation }

    Component.onCompleted: recommendation.generate()

    RecommendationElement {
        anchors.fill: parent
        model: recommendation
    }
}
