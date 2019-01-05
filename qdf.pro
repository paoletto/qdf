TEMPLATE = app

QT += qml quickcontrols2
QT += pdf quick-private
QT += widgets #for Qt labs platform
CONFIG += c++11
CONFIG += qtquickcompiler

HEADERS += $$files($$PWD/src/*.h)
SOURCES += $$files($$PWD/src/*.cpp)

qmlres.files = $$files($$PWD/src/qml/*.qml)
qmlres.prefix = /
qmlres.base = $$PWD/src/qml

jsres.files = $$files($$PWD/src/js/*.js)
jsres.prefix = /
jsres.base = $$PWD/src/js

assets.files = $$files($$PWD/assets/* , true)
assets.prefix = /
assets.base = $$PWD

RESOURCES += qmlres jsres assets
OTHER_FILES += $${qmlres.files} $${jsres.files} #$$PWD/qmldir

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

