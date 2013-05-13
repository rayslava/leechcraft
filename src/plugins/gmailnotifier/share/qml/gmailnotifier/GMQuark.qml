import QtQuick 1.1
import org.LC.common 1.0
import "lcqml:org/LC/common/Common.js" as Common

Rectangle {
    id: rootRect

    visible: GMN_proxy.msgCount > 0

    width: GMN_proxy.msgCount > 0 ? parent.quarkBaseSize : 0
    height: width

    color: "transparent"

    ActionButton {
        id: gmailButton

        anchors.fill: parent
        actionIconURL: "qrc:/gmailnotifier/gmailicon.svg"
        actionIconScales: false

        overlayText: GMN_proxy.msgCount <= 99 ? GMN_proxy.msgCount : "+"

        onTriggered: Common.showTooltip(rootRect, function(x, y) { GMN_proxy.showMailList(x, y, quarkProxy.getWinRect()) })
    }
}
