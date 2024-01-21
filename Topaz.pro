QT -= gui
QT += network
QT += sql

CONFIG += c++20 console
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../../Common/Headers
DEPENDPATH += $$PWD/../../Common/Headers

LIBS+= -L$$PWD/../../Common/Lib -lCommon

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        queryqueue.cpp \
        tconfig.cpp \
        tcoupons.cpp \
        tdoc.cpp \
        tgetdocs.cpp \
        tinputact.cpp \
        tnewsmena.cpp \
        toffact.cpp \
        tquery.cpp \
        ttopaz.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    queryqueue.h \
    tconfig.h \
    tcoupons.h \
    tdoc.h \
    tgetdocs.h \
    tinputact.h \
    tnewsmena.h \
    toffact.h \
    tquery.h \
    ttopaz.h

DISTFILES += \
    ToDo

RC_ICONS += res/Topaz.ico
