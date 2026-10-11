import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir
import Vestir.DesignTokens as Tokens

Page {

    Recommendation { id: recommendation }

    RecommendationElement {
        anchors.fill: parent
        model: recommendation
    }
}
