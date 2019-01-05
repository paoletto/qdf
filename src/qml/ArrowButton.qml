/*
Copyright (C) 2023- Paolo Angelelli <paoletto@gmail.com>

This work is licensed under the terms of the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

In addition to the above,
- The use of this work for training artificial intelligence is prohibited for both commercial and non-commercial use.
- Any and all donation options in derivative work must be the same as in the original work.
- All use of this work outside of the above terms must be explicitly agreed upon in advance with the exclusive copyright owner(s).
- Any derivative work must retain the above copyright and acknowledge that any and all use of the derivative work outside the above terms
  must be explicitly agreed upon in advance with the exclusive copyright owner(s) of the original work.
*/

import QtQuick 2.12
import QtQuick.Shapes 1.0
import QtQuick.Controls 2.5 as Controls

Shape {
    id: back
    z: parent.z + 3
    visible: true
    width: 64 * qdfContext.dpr
    height: 48 * qdfContext.dpr
    vendorExtensionsEnabled: false
    smooth: true
    signal clicked

    ShapePath {
        id: back_sp
        strokeWidth: 6 * qdfContext.dpr
        strokeColor: "black"
        fillColor: 'transparent'
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin

        property point vcenter: Qt.point(back.width * 0.5, back.height * 0.5)
        property real radius: back.width * 0.5

        property point vtop: Qt.point(vcenter.x, 0 )
        property point vbottom: Qt.point(vcenter.x, back.height )
        property point vbottomLeft: Qt.point(0, back.height)
        property point vbottomRight: Qt.point(back.width, back.height)
        property point vleft: Qt.point(0, vcenter.y)

        startX: vtop.x;
        startY: vtop.y

        PathLine { x: back_sp.vleft.x; y: back_sp.vleft.y }
        PathLine { x: back_sp.vbottom.x; y: back_sp.vbottom.y }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            back.clicked()
        }
    }
}
