import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir
import Vestir.DesignTokens as Tokens

Page {
    required property RecommendationPageModel recommendationPageModel

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ColumnLayout {
            width: scrollView.availableWidth
            Repeater {
                model: recommendationPageModel.outfits
                delegate: RecommendationElement {
                    required property OutfitRecommendation modelData
                    Layout.fillWidth: true
                    model: modelData
                }
            }
        }
    }
}
