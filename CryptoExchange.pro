QT       += core gui network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    callout.cpp \
    clickablewidgettrending.cpp \
    coin.cpp \
    coinmarketapi.cpp \
    coinpage.cpp \
    customchartview.cpp \
    discoverwindow.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    callout.h \
    clickablewidgettrending.h \
    coin.h \
    coinmarketapi.h \
    coinpage.h \
    customchartview.h \
    discoverwindow.h \
    mainwindow.h

FORMS += \
    coinpage.ui \
    discoverwindow.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
