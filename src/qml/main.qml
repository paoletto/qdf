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

import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.12 as Controls
import Qt.labs.platform 1.1 as Platform
import QtQuick.Layouts 1.12
import Qdf 1.0
import Qt.labs.settings 1.0

Window {
    id: win
    visible: true
    width: 854
    height: 854
    title: qsTr("Qdf")

    QtObject {
        id: qdfContext
        readonly property real pixelDensityDesktop: 3.7
        property real  pixelDensity: pixelDensityDesktop
        property int screenWidth
        property int screenHeight
        property real dpr : Math.max(1.0, pixelDensity / pixelDensityDesktop)

        function update(size, pixel_density) {
            pixelDensity = pixel_density
            screenWidth = size.width
            screenHeight = size.height
        }

        function devicePixelRatio() {
            if (pixelDensity == undefined) {
                return 1.0
            } else {
                return Math.max(1.0, pixelDensity / pixelDensityDesktop)
            }
        }

        property int _TINY_FONT_SIZE: 10 * dpr
        property int _XSMALL_FONT_SIZE: 14 * dpr
        property int _SMALL_FONT_SIZE: 18 * dpr
        property int _SEMISMALL_FONT_SIZE: 20 * dpr
        property int _STANDARD_FONT_SIZE: 24 * dpr
        property int _LARGE_FONT_SIZE: 32 * dpr
        property int _HUGE_FONT_SIZE: 40 * dpr
        property int _MARGIN: 20
        property int _NUMBER_TEXTFIELD_WIDTH: 85
        property string _TEXT_COLOR: "white" //theme.inverted ? "white" : "black"
        property string _ICON_LOCATION: "qrc:/assets/icons/"
        property string _IMAGE_LOCATION: "qrc:/assets/images/"
        property string _ACTIVE_COLOR_TEXT: "#8d68be" // "#8D18BE" // purple
        property string _DISABLED_COLOR_TEXT: "#b4b4cc"

        property var dynamicProperties: QmlObj {}
    }


    property real dpr:  qdfContext.dpr
    Settings {
        id: settingsMargins
        category: "cropSettings"

        function storeMargins(fname, bytecounts, margins) {
            console.log("storeMargins")
            if (!fname || !bytecounts) {
                console.log(fname, bytecounts)
                return
            }
            setValue(fname+bytecounts, margins)
            sync()
            console.log(fileName)
        }

        function loadMargins(fname, bytecounts) {
            console.log("loadMargins")
            if (!fname || !bytecounts) {
                console.log(fname, bytecounts)
                return []
            }
            return value(fname+bytecounts, [])
        }
    }

    Settings {
        id: settingsPosition
        category: "positionSettings"

        function storePositionData(fname, bytecounts, posData) {
            if (!fname || !bytecounts) {
                console.log(fname, bytecounts)
                return
            }
            setValue(fname+bytecounts, posData)
            sync()
        }

        function loadPositionData(fname, bytecounts) {
            console.log("loadPositionData")
            if (!fname || !bytecounts) {
                console.log(fname, bytecounts)
                return {}
            }
            return value(fname+bytecounts, {})
        }
    }

    Timer {
        id: settingsPositionTimer
        interval: 5000; running: false; repeat: false
        onTriggered: {
            var posData = {}
            posData["x"] = pdfView.contentX
            posData["y"] = pdfView.contentY
            posData["zoom"] = pdfView.zoom
            settingsPosition.storePositionData(pdfView.fileName, pdfView.bytesCount, posData)
        }
    }

    function updateDpr() {
        // Filling ctx
        qdfContext.update(Qt.size(Screen.width, Screen.height), Screen.pixelDensity)
        qdfContext.dynamicProperties.menuButtonFontSize = qdfContext._SEMISMALL_FONT_SIZE
    }

    Component.onCompleted: {
        win.updateDpr()
        console.log("Completed!" , Screen.pixelDensity, qdfContext.devicePixelRatio(), qdfContext.dpr)
    }

    Screen.onPixelDensityChanged: {
        win.updateDpr()
    }

    onVisibilityChanged: { // fallback for emergencies
        win.updateDpr()
    }

    property real uiRotation : 0
    property var appOrientation:
        (uiRotation == -90 || uiRotation == -270
         || uiRotation == 90 || uiRotation == 270) ?
        Qt.Vertical : Qt.Horizontal
    property alias openDialog: openDialog

    Platform.FileDialog {
        id: openDialog
        objectName: "openProjectDialog"
        nameFilters: ["All files (*)", "PDF files (*.pdf)"]
        defaultSuffix: "pdf"
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DesktopLocation)
        Component.onCompleted: console.log(folder)
        onAccepted: {
            pdfView.documentPath = file
            cropper.margins = settingsMargins.loadMargins(pdfView.fileName, pdfView.bytesCount)
            var posData = settingsPosition.loadPositionData(pdfView.fileName, pdfView.bytesCount)
            // use posData
            pdfContainer.setZoom(isFinite(posData["zoom"]) ? posData["zoom"] : 1)
            pdfView.contentY = isFinite(posData["y"]) ? posData["y"] : 0
            pdfView.contentX = isFinite(posData["x"]) ? posData["x"] : 0
        }
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: {
            openDialog.visible = true
        }
    }

    Shortcut {
        sequence: StandardKey.Quit
        onActivated: {
            Qt.quit()
        }
    }

    Shortcut {
        sequence: "Ctrl+D"
        onActivated: {
            pdfView.printDebug()
        }
    }


//    Dialogs.FileDialog {
//        id: openDialog
//        objectName: "openProjectDialog"
//        title: "Please choose a file"
//        nameFilters: ["All files (*)", "PDF files (*.pdf)"]
//        onAccepted: {
//            console.log("You chose: " + openDialog.fileUrls)
//            pdfView.documentPath = openDialog.fileUrls[0]
//        }
//        onRejected: {
//            console.log("Canceled")
//        }
////        Component.onCompleted: visible = true
//    }

//    FileSelector {
//        id: openDialog
//        visible: false
//        anchors.fill: parent
//    }

    Shortcut {
        sequence: "Ctrl+P"
        onActivated: {
            console.log("curIdx: ",pdfView.currentIndex)

        }
    }

    Shortcut {
        sequence: "Ctrl+Z"
        onActivated: {
            sli.value = 2

        }
    }




    Rectangle {
        id: stackviewPdfFrame
        objectName: "stackviewPdfFrame"
        color: "transparent"

        Rectangle {
            id: pdfContainer
            objectName: "pdfContainer"
            color: "transparent"
            rotation: uiRotation
            transformOrigin: Item.Center
            anchors.centerIn: parent
            width: (orientation === Qt.Horizontal) ? win.width : win.height
            height: (orientation === Qt.Horizontal) ? win.height : win.width
            property var orientation: win.appOrientation

            function setPdfViewWidth() {
                if (pdfContainer.width > 0 && pdfContainer.width > pdfView.contentWidth)
                    pdfView.setDocumentWidth(pdfContainer.width)
            }
            function refocus() {
//                if (pdfContainer.width > 0 && sli.value > 0)
//                    pdfView.setContentWidth(pdfContainer.width * sli.value)
                pdfView.refocus()
            }

            function setZoom(zm) {
                pdfView.zoom = zm
                pdfView.refocus()
            }

            onWidthChanged: {
//                Qt.callLater(setPdfViewWidth)
                setPdfViewWidth()
            }

            PdfView {
                id: pdfView
                anchors.fill: parent
                dpr: qdfContext.dpr
                onDoubleTap: {
                    toolbar.visible = !toolbar.visible
                }
                margins: cropper.margins

                onCurrentIndexChanged: {
                    settingsPositionTimer.restart()
                }

                onContentYChanged: {
//                    storePosData()
//    //                console.log("curIdx at",
//    //                            pdfView.contentY, ": ",
//    //                            pdfView.indexAt(pdfView.contentY));
////                    console.log("curImg: ", pdfView.currentImage())

//                    var curImg = pdfView.currentImage()
//                    if (curImg !== miniMap.source)
//                        miniMap.source = curImg

//                    console.log("MM SS:",miniMap.sourceSize)
////                    var curIndex = pdfView.indexAt(pdfView.contentY)
////                    miniMap.source = "image://pdfpages/0/" + (curIndex + 3)
                }
                onZoomChanged: {
                    settingsPositionTimer.restart()
                }
            }


//            Slider {
//                id: sli
//                orientation: Qt.Horizontal
//                anchors {
//                    left: parent.left
//                    right: parent.right
//                    bottom: parent.bottom
//                    leftMargin: 20
//                    rightMargin: 20
//                    bottomMargin: 20
//                }
//                from: 1.0
//                to: 8.0
//                onValueChanged: {
//                    if (sli.pressed)
//                        pdfContainer.setZoom(value)
//                }
//                onPressedChanged: {
//                    if (!sli.pressed) {
//                        // refocus
//                        //pdfContainer.setPdfContentWidth()
//                    }
//                }
//            }


            Rectangle {
                id: toolbar
                visible: false
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }
                height: 64 * qdfContext.dpr
                color: Qt.rgba(1,0,1,0.1)
                RowLayout {
                    id: toolbarLayout
                    anchors.fill: parent
                    spacing: 6 * qdfContext.dpr
                    Controls.Button {
                        text: "Open"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            toolbar.visible = false
//                            openDialog.open()
                            openDialog.visible = true
                        }

                    }
                    Controls.Button {
                        text: "R-90"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            uiRotation = (uiRotation - 90) % 360
                        }
                    }
                    Controls.Button {
                        text: "R+90"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            uiRotation = (uiRotation + 90) % 360
                        }
                    }
                    Controls.Button {
                        text: "Fit"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            toolbar.visible = false
                            pdfView.fit()
                        }
                    }

                    Controls.Button {
                        text: "Fullscreen"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            toolbar.visible = false
                            win.visibility = (win.visibility === Window.Windowed) ?
                                                    win.visibility = Window.FullScreen :
                                                    win.visibility = Window.Windowed
                            pdfView.fit()
                        }
                    }

                    Controls.Button {
                        text: "Invert"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            pdfView.invert = !pdfView.invert
                        }
                    }

                    Controls.Button {
                        text: "Crop"
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: qdfContext.dynamicProperties.menuButtonFontSize
                        onClicked: {
                            cropper.pageCount = pdfView.pageCount
                            cropper.pageSize = pdfView.pageSize
                            cropper.documentId = pdfView.documentId
                            cropper.pageIndex = pdfView.indexAt(pdfView.contentY)
                            cropper.pushMargins(settingsMargins.loadMargins(pdfView.fileName, pdfView.bytesCount))
                            pageStack.push(cropperFrame)
                            cropperFrame.enabled = cropperFrame.visible = true

                        }
                    }
                }
            }

            states: State {
                name: "toolbar_on"
                AnchorChanges { target: toolbar; anchors.bottom: undefined }
                AnchorChanges { id: acMoveIn; target: toolbar; anchors.top: pdfContainer.top }
            }

            transitions: [
                    Transition {
                    // smoothly reanchor myRect and move into new position
                    AnchorAnimation { targets: acMoveIn; duration: 1000 }
                }
            ]


        } // Rectangle // pdf container
    } // Rectangle // stackview frame

    Rectangle {
        id: cropperFrame
        color: "transparent"
        enabled: false
        visible: false
        objectName: "cropperFrame"
        PageCropper {
            id: cropper
            invert: pdfView.invert

            rotation: uiRotation
            transformOrigin: Item.Center
            anchors.centerIn: parent
            property var orientation: win.appOrientation
            width: (orientation === Qt.Horizontal) ? parent.width : parent.height
            height: (orientation === Qt.Horizontal) ? parent.height : parent.width

            onPopped: {
                pageStack.pop()
                cropper.pullMargins()
                settingsMargins.storeMargins(pdfView.fileName, pdfView.bytesCount, cropper.margins)
                cropperFrame.enabled = cropperFrame.visible = false

            }
        }
        StackView.onActivated: cropper.activated()
        StackView.onDeactivated: cropper.deactivated()
    }

    Controls.StackView {
        id: pageStack
        anchors.fill: parent
        initialItem: stackviewPdfFrame

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }

        Component.onCompleted: {
            console.log(depth)
            console.log(currentItem.objectName)
        }
    } // StackView
//    Image {
//        id: miniMap
//        anchors.bottom: parent.bottom
//        anchors.right: parent.right
//        width: 200
//        height: 200
//        sourceSize: (pdfView.pageSize !== undefined)
//                    ? pdfView.pageSize
//                    : Qt.size(100,100)
//        source: "file:///home/user/black.jpeg"
//    }
}
