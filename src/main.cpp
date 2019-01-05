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

#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtPlugin>
#include <QQmlExtensionPlugin>
#include <QQmlContext>
#include <QDirIterator>
#include <QtQml/qqml.h>
#include <QtQml/qqmlpropertymap.h>
#include <QDebug>

#include "flickablegesturearea.h"
#include "pdfimageprovider.h"
#include "qquickflickerlessimage.h"

class DragDistanceChanger: public QObject
{
    Q_OBJECT

public:
    DragDistanceChanger(QObject *parent = 0) : QObject(parent)
    {
        m_initialDragDistance = qQNaN();
    }


public slots:
    void changeDragDistance() {
        qreal dpr = m_win->property("dpr").toReal();
        qDebug() << "changeDragDistance" << dpr;
        if (!qIsFinite(m_initialDragDistance))
            m_initialDragDistance = QApplication::startDragDistance();
        QApplication::setStartDragDistance(m_initialDragDistance * dpr);
    }

public:
    QObject *m_win = nullptr;
    qreal m_initialDragDistance;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("qdf.pw");
    QApplication app(argc, argv);


    QQmlApplicationEngine engine;
    const char *uri = "Qdf";
    const int major = 1;
    const int minor = 0;
    qmlRegisterType<QQuickImage, 2>(uri, major, minor, "_Image");
    qmlRegisterType<PdfManager>(uri, major, minor, "PdfManager");
    qmlRegisterType<QQuickFlickerlessImage>(uri, major, minor, "FlickerlessImage");
    qmlRegisterType<FlickableGestureArea>(uri, major, minor, "FlickableGestureArea");
    qmlRegisterType<QQmlPropertyMap>(uri, major, minor, "QmlObject");

    PdfImageProvider &provider = PdfImageProvider::instance();
    engine.addImageProvider("pdfpages", &provider);

#if defined(Q_OS_ANDROID)
    engine.rootContext()->setContextProperty(QStringLiteral("platform_name"), QVariant::fromValue(QStringLiteral("mobile")));
#else
    engine.rootContext()->setContextProperty(QStringLiteral("platform_name"), QVariant::fromValue(QStringLiteral("desktop")));
#endif
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    QObject *root = engine.rootObjects().first(); // the window
    DragDistanceChanger dragChanger(&app);
    dragChanger.m_win = root;
    dragChanger.connect(root, SIGNAL(dprChanged()), SLOT(changeDragDistance()));
    dragChanger.changeDragDistance();

//    QFile frag(":/assets/shaders/texture_frag.glsl");
//    frag.open(QIODevice::ReadOnly | QIODevice::Text);
//    qDebug().noquote() << frag.readAll();

    return app.exec();
}
