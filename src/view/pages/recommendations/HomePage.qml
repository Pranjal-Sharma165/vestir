import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir
import Vestir.DesignTokens as Tokens

Page {
    required property RecommendationPageModel recommendationPageModel

    ColumnLayout {
        anchors.fill: parent

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: recommendationPageModel.current !== null

            Loader {
                active: !!recommendationPageModel.current
                visible: active
                sourceComponent: RecommendationElement {
                    width: scrollView.availableWidth
                    model: recommendationPageModel.current
                    decided: recommendationPageModel.chosen
                    onNotForMe: recommendationPageModel.notForMe()
                    onWearToday: recommendationPageModel.choose()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: recommendationPageModel.current === null
            visible: recommendationPageModel.current === null
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: recommendationPageModel.noMore
                  ? "No more outfits to suggest for now."
                  : "Finding outfits..."
        }

    }
}
