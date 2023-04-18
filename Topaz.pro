QT -= gui
QT += network
QT += sql

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        Common/common.cpp \
        Common/regcheck.cpp \
        Common/tdbloger.cpp \
        Common/thttpquery.cpp \
        main.cpp \
        tconfig.cpp \
        tcoupons.cpp \
        tdoc.cpp \
        tgetdocs.cpp \
        tinputact.cpp \
        tnewsmena.cpp \
        toffact.cpp \
        ttopaz.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Common/common.h \
    Common/regcheck.h \
    Common/tdbloger.h \
    Common/thttpquery.h \
    tconfig.h \
    tcoupons.h \
    tdoc.h \
    tgetdocs.h \
    tinputact.h \
    tnewsmena.h \
    toffact.h \
    ttopaz.h

DISTFILES += \
    ToDo

RC_ICONS += res/Topaz.ico
