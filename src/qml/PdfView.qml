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
import QtQuick.Window 2.2
import Qdf 1.0
import Qt.labs.platform 1.0 as Platform

Rectangle {
    enum ViewMode {
        Continuous1Up,
        Continuous2Up
    }
    property int viewMode : PdfView.ViewMode.Continuous1Up
    onViewModeChanged: {
        if (viewMode == PdfView.ViewMode.Continuous1Up) {
//            currentView.enabled = false // possibly not needed, as enabled prop is bound
            currentView = pagesView
//            currentView.enabled = true // possibly not needed, as enabled prop is bound
        } else if (viewMode == PdfView.ViewMode.Continuous2Up) {
//            currentView = listView2Up
        }
    }

    id: pdfView
    color: (pdfView.invert) ? "black" : "transparent"
    function setDocumentWidth(w) // this sets both the content width and the pdf width
    {
        console.log("PdfView -- setDocumentWidth",w)
        setFlickableWidth(w)
        setRasterWidth(w)
    }

    function setFlickableWidth(w) {
        if (w < pdfView.width) // cant set it smaller than the phyisical window width
            return;
        console.log("setting contentWidth",w)
        contentWidth = w  // contentWidth of the pages (delegates)
    }

    function setRasterWidth(w) { // sets width of the images
        console.log("PdfView -- setRasterWidth",w)
        pdfWidth = w
    }

    function refocus() {
//        if (pdfWidth < w) // without bilinear filtering no scale down looks crappy
                            // However, even with bilinear filtering and mipmap, scale down is lower quality
                            // Plus, there's a considerable performance hit with large pics
        setRasterWidth(contentWidth) // setting it to the width of the page.
                                               // ToDo: clamp?
    }

    onWidthChanged: {
        console.log("Width changed: ", width)
    }

    function fit() {
        setFlickableWidth(pdfView.width)
        setRasterWidth(pdfView.width)
    }

    property var documentModel : []
    // This object needs contentX, contentY, contentWidth and currentIndex
    property var currentView: pagesView

    Platform.FileDialog {
        id: openDialog
        objectName: "openProjectDialog"
        nameFilters: ["All files (*)", "PDF files (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: {
            if (pdfView.documentId >= 0) {
                pdfView.documentModel = []
                pdfManager.closeDocument(pdfView.documentId)
                pdfView.documentId = -1
                pdfView.pageCount = 0
                pdfView.bytesCount = 0
                pdfView.fileName = ""
            }
            pdfView.documentPath = file
        }
    }

    property var pageSize: Qt.size(0,0)
    property bool invert: false
    property real pdfWidth: 0
    property real scaleFactor: pdfWidth / pdfView.width
    property string documentPath

    property alias zoom: pageGestureHandler.scale
    property real scale: 1.0
    property real dpr: 1.0

    onScaleChanged: {
        setFlickableWidth(pdfView.width * zoom) // will refocus on release
    }

    property alias contentX: pagesView.contentX
    property alias contentY: pagesView.contentY
    property int documentId : -1
    property int pageCount: 0
    property int bytesCount: 0
    property string fileName
    property alias contentWidth: pagesView.contentWidth
    property alias currentIndex: pagesView.currentIndex

    // needed to update the margins atomically
    // Intended as left,right,top,bottom
    // in percentage.
//    property vector4d margins: Qt.vector4d(0,0,0,0)
    property var margins : []
    function _margins(idx) {
        if ((idx < 0)
                || (margins === undefined)
                || (idx >=  margins.length))
            return Qt.vector4d(0,0,0,0)
        return margins[idx].margins
    }

    function _marginString(idx)
    {
        var mrgs = _margins(idx)
        return "[" + mrgs.x.toFixed(2)
             + "," + mrgs.y.toFixed(2)
             + "," + mrgs.z.toFixed(2)
             + "," + mrgs.w.toFixed(2)
             + "]"
    }

    function currentImageSource() {
        if (!documentModel.documentModel)
            return ""
        return pagesView.itemAt(0, pdfView.contentY).imageSource;
    }

    function indexAt(y, x = 0) {
        return pagesView.indexAt(x, y);
    }

    function itemAt(y, x = 0) {
        return pagesView.itemAt(x, y);
    }

    signal doubleTap
    onDocumentPathChanged: {

        var cleanPath = documentPath.replace(/^(file:\/{2})/,"");
        console.log("PdfView -- ","PdfManager: Loading", cleanPath)
        pdfManager.openDocument(cleanPath)
    }

    function printDebug()
    {
        console.log("==== PDFVIEW ====")
        console.log("current index:", pagesView.currentIndex)
        console.log("handler status:", pageGestureHandler.currentIndex )
        console.log("**** ******* ****")
    }

    PdfManager {
        id: pdfManager

        onReady: {
            console.log("PdfView -- onReady","document ",documentId, "ready")
            pdfView.documentId = documentId;
            pdfView.pageCount = pdfManager.pageCount(documentId)
            pdfView.bytesCount = pdfManager.bytesCount(documentId)
            pdfView.fileName = pdfManager.fileName(documentId)
            var sz = pdfManager.pageSize(documentId, 0)
            console.log("META:",JSON.stringify(pdfManager.metadata(documentId)))
            pdfView.pageSize = sz;
            if (pdfView.width != 0) {
                refocus()
            } else {
                setFlickableWidth(sz.width)
                setRasterWidth(sz.width)
            }

            pdfView.documentModel = pdfManager.pages(documentId)
            console.log("SZ:",sz.width, sz.height)
        }
        Component.onCompleted: {
        }
    }

    ListView {
        id: pagesView;
        enabled: pdfView.viewMode == PdfView.ViewMode.Continuous1Up
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        anchors.fill: parent
        flickableDirection: Flickable.AutoFlickDirection
        model: pdfView.documentModel

//        Binding {
//            target: pdfView
//            property: "contentX"
//            value: pagesView.contentX
//        }
//        Binding {
//            target: pdfView
//            property: "contentY"
//            value: pagesView.contentY
//        }

        delegate: Column {
            spacing: 0
            property real cropped_ar: page1up.croppedAR(modelData.page_ar)
            FlickerlessImage {
                id: page1up
                //            anchors.fill: parent
                //            asynchronous: false
                invert: pdfView.invert
                cache: false
                smooth: width !== sourceSize.width // defaults to true
                property string imageSource: modelData.image
                source: modelData.image + "/" + pdfView._marginString(index) + "/pagesViewDelegate"

                width: pagesView.contentWidth // contentWidth is the same for all pages
                height: pagesView.contentWidth / cropped_ar

                function croppedAR(ar) {
                    var mrgs = pdfView._margins(index)
                    return (mrgs) ? (1.0 - mrgs.x - mrgs.z) / ((1.0 - mrgs.y - mrgs.w) / ar)
                                  : ar
                }

                sourceSize.width: pdfWidth
                sourceSize.height: pdfWidth / modelData.page_ar

//                Component.onCompleted: {
//                     console.log("PdfView -- ",modelData, modelData.image, modelData.page_width, modelData.page_height, pagesView.contentWidth)
//                }

                Rectangle {
                    id: pageNumber
                    width: 20 * dpr
                    height: width
                    radius: width * 0.5
                    color: "tomato"
                    opacity: 0.6
                    anchors {
                        right: parent.right
                        top: parent.top
                        topMargin: 10 * dpr
                        rightMargin: 10 * dpr
                    }

                    Text {
                        font.pixelSize: 13 * dpr
                        anchors.centerIn: parent
                        text: "" + index
                    }
                }
            }
            Rectangle {
                id: pageDelimiter
                height: 2
                width: page1up.width * 0.8
                anchors.horizontalCenter: parent.horizontalCenter
                color: "firebrick"
            }
        }

        function reanchor(idx, normalizedPositionInPage, pixel) {
            // step1: jump to old current index
//            console.log("reanchor",pagesView.currentIndex,idx, normalizedPositionInPage,pixel)
            if (pagesView.currentIndex != idx)
                pagesView.currentIndex = idx
            var itm = pagesView.itemAtIndex(pagesView.currentIndex)
            if (!itm)
                return
            // step2: reanchor normalizedPositionInPage at centroidPosition
            //var pageSize = Qt.size(itm.width, itm.height)
            var pageSize = Qt.size(pagesView.contentWidth, pagesView.contentWidth / itm.cropped_ar)


            var absolutePointPosition = Qt.point(itm.x +
                                                 normalizedPositionInPage.x * pageSize.width,
                                                 itm.y +
                                                 normalizedPositionInPage.y * pageSize.height)
            var newContentPosition = Qt.point(absolutePointPosition.x - pixel.x,
                                              absolutePointPosition.y - pixel.y)

//            console.log("xy:",pagesView.contentX,
//                        pagesView.contentY,
//                        newContentPosition,
//                        pageSize,
//                        pagesView.contentWidth)
            pagesView.contentX = newContentPosition.x
            pagesView.contentY = newContentPosition.y
        }


        FlickableGestureArea {
            id: pageGestureHandler
            anchors.fill: parent
            minimumScale: 1.0
            maximumScale: 8.0

            property point normalizedPosition: Qt.point(0,0)
            property point normalizedPositionInPage: Qt.point(0,0)
            property int currentIndex: 0

            function zoom() {
                var pt = Qt.point(pagesView.contentX+centroid.x,
                                  pagesView.contentY+centroid.y)
                var itm = pagesView.itemAt(pt.x, pt.y)
                var itmPos = Qt.point(itm.x, itm.y)


                currentIndex = pagesView.indexAt(pt.x, pt.y)
                normalizedPositionInPage = Qt.point( (pt.x - itmPos.x) / itm.width,
                                                     (pt.y - itmPos.y) / itm.height )
//                console.log("CI: ", currentIndex)
//                console.log("PosInPg:", normalizedPositionInPage, centroid)
                pdfView.scale = scale
            }

            onScaleChanged: {
                zoom()
            }

            onActiveChanged: {
                if (active) {
                    pinchCentroid.show(centroid)
                } else {
                    pinchCentroid.hide()
                    pdfView.refocus()
                }
            }

            onCentroidChanged: {
                pinchCentroid.move(centroid)
            }

            function adjustXY() {
                // step1: jump to old current index
                pagesView.reanchor(pageGestureHandler.currentIndex, normalizedPositionInPage, centroid)
            }

            onDoubleClicked: {
                pdfView.doubleTap()
            }
        }

        onContentHeightChanged: {
            // called asynchronously after scale has been set onto pagesView
            if (!pageGestureHandler.active && !pageGestureHandler.wheeled) {
                return
            }
//            console.log("CH Changed")
            pageGestureHandler.wheeled = false;
            pageGestureHandler.adjustXY()
        }

        Rectangle {
            id: pinchCentroid
            color: "firebrick"
            opacity: 0.4
            width: 16
            height: width
            radius: width * 0.5
            visible: false
            function show(p) {
                move(p)
                visible = true
            }
            function hide() {
                visible = false
            }
            function move(p) {
                x = p.x - radius
                y = p.y - radius
            }
        }
    } // pagesView ListView (1up)
}
