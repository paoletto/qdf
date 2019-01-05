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
import QtQuick.Controls 2.5
import QtQuick.Controls 2.5 as Controls
import Qt.labs.platform 1.0 as Platform
import QtQuick.Layouts 1.12
import QtQuick.Shapes 1.0
import Qdf 1.0 // for the fancy Image
import QtGraphicalEffects 1.0

Rectangle {
    id: root
//    color: "transparent"
    color: "gray"



    property real topMargin: -1
    property real bottomMargin: -1
    property real leftMargin: -1
    property real rightMargin: -1
    property string marginsMode: "s" // , "eo", "i"
    property var imageSource: undefined
    property int pageIndex: -1
    property int documentId: -1
    property int pageCount: 0
    property bool invert: false
    property var pageSize: Qt.size(1,1)
    signal popped


//    property vector4d margins: Qt.vector4d(0,0,0,0)
    property var margins: []
    property var _margins: []
    onPageCountChanged: {
        // resize _margins, thanks JS
        while(root.pageCount > _margins.length) {
            var entry = {}
            entry.mode = "s"
            entry.margins = Qt.vector4d(0,0,0,0)
            _margins.push(entry);
        }
        _margins.length = root.pageCount;
    }

    function computeMargins() {
        return Qt.vector4d(Math.max(0, leftMargin),
                                      Math.max(0, topMargin),
                                      Math.max(0, rightMargin),
                                      Math.max(0, bottomMargin))
    }

    function pushMargins(m) {
        if (m.length != root.pageCount) {
            _margins = []
            while(root.pageCount > _margins.length) {
                var entry = {}
                entry.mode = "s"
                entry.margins = Qt.vector4d(0,0,0,0)
                _margins.push(entry);
            }
        } else {
            _margins = m
            changePageIndex(pageIndex)
        }
    }

    function pullMargins() {
//        margins = computeMargins()
        root.margins = root._margins
    }
    function _imageSource(did, pidx) {
        if ((did < 0) || (pidx < 0) || (pidx >= root.pageCount))
            return "";
        return "image://pdfpages/" + did + "/" + pidx + "/Cropper"
    }

    enum MarginsMode {
        Single,
        Even_Odd,
        Individual
    }

    property int _mode : PageCropper.MarginsMode.Single
    function _storeMargins()
    {
        if (root._mode == PageCropper.MarginsMode.Single)
        {
            for (var i = 0; i < _margins.length; i++)
                if (_margins[i].mode === "s")
                    _margins[i].margins = computeMargins()
        }
        else if (root._mode == PageCropper.MarginsMode.Even_Odd)
        {
            var oddness = pageIndex % 2
            for (var i = 0; i < _margins.length; i++)
            {
                var mode = _margins[i].mode
                if ((i%2) === oddness && (mode === "eo" || mode === "s")) {
                    _margins[i].mode = "eo"
                    _margins[i].margins = computeMargins()
                }
            }
        }
        else
        {
            _margins[pageIndex].mode = "i"
            _margins[pageIndex].margins = computeMargins()
        }
    }

    function _loadMargins(i)
    {
//        if (root._mode == PageCropper.MarginsMode.Single)
//        {

//        }
//        else if (root._mode == PageCropper.MarginsMode.Even_Odd)
//        {

//        }
//        else
//        {

//        }
        var mrg = _margins[i].margins
        leftMargin = mrg.x
        topMargin = mrg.y
        rightMargin = mrg.z
        bottomMargin = mrg.w
    }

    function changePageIndex(idx) {
        root._loadMargins(idx)
        root.repositionHandles()
    }

    onPageIndexChanged: {
        changePageIndex((pageIndex))
    }

    onWidthChanged: repositionHandles()
    onHeightChanged: repositionHandles()
    function repositionHandles() // called onSourceSizeChanged
    {
        root.activating = true
        left.x = Math.max(0, root.leftMargin) * fi.width - left.radius
        right.x = fiBg.width - Math.max(0, root.rightMargin) * fi.width - right.radius
        top.y = Math.max(0, root.topMargin) * fi.height - top.radius
        bottom.y = fiBg.height - Math.max(0, root.bottomMargin) * fi.height - bottom.radius

//        left.y = (0 + Math.max(0, root.topMargin)
//                  + fiBg.height - Math.max(0, root.bottomMargin)) * 0.5
//                  - root.handleRadius

//        top.x = (0 + Math.max(0, root.leftMargin)
//                 + fiBg.width - Math.max(0, root.rightMargin)) * 0.5
//                 - root.handleRadius
        left.y = (top.center.y + bottom.center.y) * 0.5 - root.handleRadius
        top.x = (left.center.x + right.center.x) * 0.5 - root.handleRadius

        right.y = left.y
        bottom.x = top.x
        root.activating = false
    }

    ArrowButton {
        id: back
        anchors {
            left: parent.left
            top: parent.top
            leftMargin: 12 * qdfContext.dpr
            topMargin: 12 * qdfContext.dpr
        }
        onClicked: {
            root._storeMargins()
//            root.fillMargins() // exposes the margins
            root.popped()
        }
    }

    ArrowButton {
        id: previous
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            leftMargin: 10 * qdfContext.dpr
        }
        width: 0.7 * 64 * qdfContext.dpr
        height: 0.7 * 48 * qdfContext.dpr
        onClicked: {
            if (root.pageIndex < 0)
                return;
            root._storeMargins()
            root.pageIndex = Math.max(0, root.pageIndex - 1)
        }
    }

    ArrowButton {
        id: next
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            rightMargin: 10 * qdfContext.dpr
        }
        rotation: 180
        width: 0.75 * 64 * qdfContext.dpr
        height: 0.75 * 48 * qdfContext.dpr
        onClicked: {
            if (root.pageIndex < 0)
                return;
            root._storeMargins()
            root.pageIndex = Math.max(0, root.pageIndex + 1)
        }
    }


    property real ar: width / height
    property real _handleBaseSize : 24
    property int _handleBaseSizeScaled: _handleBaseSize * qdfContext.dpr
    property real handleSize: (_handleBaseSizeScaled % 2)
                                ? _handleBaseSizeScaled + 1
                                : _handleBaseSizeScaled  // even sized now
    property real handleRadius: handleSize * 0.5
    Rectangle {
        id: fiBg
        color: "white"
        anchors.centerIn: parent
        width: fi.width
        height: fi.height
        FlickerlessImage {
            id: fi
            visible: true
//            source: (root.imageSource === undefined) ? "" : root.imageSource + "/Cropper"
  //        source: "file:///home/user/black.jpeg"
            source: root._imageSource(root.documentId, root.pageIndex)
            invert: root.invert
            cache: false
            // Do not change the scaling, change the size
            anchors.centerIn: parent
            property real ar: (root.pageSize === undefined)
                                ? 1
                                : root.pageSize.width / root.pageSize.height
            width: (fi.ar < root.ar)
                        ? root.height * fi.ar
                        : root.width
            height: (fi.ar > root.ar)
                        ? root.width / fi.ar
                        : root.height
            sourceSize: Qt.size(width, height)

            onSourceSizeChanged: {
                root.repositionHandles()
            }

            function alignLeftRight()
            {
                left.y = right.y =
                    (top.center.y + bottom.center.y) * 0.5 - root.handleRadius

            }

            function alignTopBottom()
            {
                top.x = bottom.x =
                    (left.center.x + right.center.x) * 0.5 - root.handleRadius
            }

            Rectangle {
                id: recGrey

                color: "gray"
//                opacity: 0.4
                anchors.fill: parent
                visible: false
            }

            Rectangle {
                id: recMask

                color: "transparent"
//                opacity: 0.0
                anchors.fill: parent
                visible: false
                Rectangle {
                    id: recVis
                    color: "green"
    //                opacity: 1.0
    //                visible: false
                    x: left.center.x
                    y: top.center.y
                    width: right.center.x - left.center.x
                    height: bottom.center.y - top.center.y
                }
            }


            OpacityMask {
                visible: true
                anchors.fill: recGrey
                source: recGrey
                maskSource: recMask
                invert: true
                opacity: 0.4
            }

            Rectangle {
                id: left
                color: "firebrick"
                width: root.handleSize
                height: width
                radius: root.handleRadius

                border.width: 0
                onXChanged: {
                    if (root.activating)
                        return;
                    root.leftMargin = left.center.x / fi.width
                    fi.alignTopBottom()
                }
                onYChanged: {

                }
                property point center: Qt.point(x+root.handleRadius,
                                                y+root.handleRadius)
                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.minimumX: 0 - left.radius - 1
                    drag.maximumX: right.x - right.width
                    drag.minimumY: (top.center.y + bottom.center.y) * 0.5 - root.handleRadius
                    drag.maximumY: drag.minimumY
                }
            }

            Rectangle {
                id: right
                color: "firebrick"
                width: root.handleSize
                height: width
                radius: root.handleRadius

                border.width: 0
                onXChanged: {
                    if (root.activating)
                        return;
                    root.rightMargin = (fiBg.width - right.center.x) / fi.width
                    fi.alignTopBottom()
                }
                onYChanged: {

                }
                property point center: Qt.point(x+root.handleRadius,
                                                y+root.handleRadius)
                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.minimumX: left.x + left.width
                    drag.maximumX: fiBg.width - left.radius + 1
                    drag.minimumY: (top.center.y + bottom.center.y) * 0.5 - root.handleRadius
                    drag.maximumY: drag.minimumY
                }
            }

            Rectangle {
                id: top
                color: "firebrick"
                width: root.handleSize
                height: width
                radius: root.handleRadius

                border.width: 0
                onXChanged: {

                }
                onYChanged: {
                    if (root.activating)
                        return;
                    root.topMargin = top.center.y / fi.height
                    fi.alignLeftRight()
                }
                property point center: Qt.point(x+root.handleRadius,
                                                y+root.handleRadius)
                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.minimumY: 0 - top.radius - 1
                    drag.maximumY: bottom.y - bottom.height
                    drag.minimumX: (left.center.x + right.center.x) * 0.5 - root.handleRadius
                    drag.maximumX: drag.minimumX
                }
            }

            Rectangle {
                id: bottom
                color: "firebrick"
                width: root.handleSize
                height: width
                radius: root.handleRadius

                border.width: 0
                onXChanged: {

                }
                onYChanged: {
                    if (root.activating)
                        return;
                    root.bottomMargin = (fiBg.height - bottom.center.y) / fi.height
                    fi.alignLeftRight()
                }
                property point center: Qt.point(x+root.handleRadius,
                                                y+root.handleRadius)
                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.minimumY: top.y + top.height
                    drag.maximumY: fiBg.height - top.radius + 1
                    drag.minimumX: (left.center.x + right.center.x) * 0.5 - root.handleRadius
                    drag.maximumX: drag.minimumX
                }
            }
        }
    }
    property bool activating: false
    function printMargins() {
        console.log("MRGS", root.leftMargin, root.topMargin, root.rightMargin, root.bottomMargin)
    }

    function activated() {
        console.log("ONACTIVATED")
        console.log("HN", root.handleSize, root.handleRadius)
        printMargins()
        repositionHandles()
        printMargins()
    }

    function deactivated() {
        printMargins()
    }

    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        spacing: 2
        Controls.RadioButton {
            checked: true
            text: qsTr("Single")
            onCheckedChanged: root._mode = PageCropper.MarginsMode.Single
        }
        Controls.RadioButton {
            text: qsTr("Even/Odd")
            onCheckedChanged: root._mode = PageCropper.MarginsMode.Even_Odd
        }
        Controls.RadioButton {
            text: qsTr("Individual")
            onCheckedChanged: root._mode = PageCropper.MarginsMode.Individual
        }
    }
}
