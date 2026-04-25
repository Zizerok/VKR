QT       += core gui
QT      += sql
QT      += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addemployeedialog.cpp \
    addshiftdialog.cpp \
    businesslist.cpp \
    businessmainwindow.cpp \
    databasemanager.cpp \
    employeecardwindow.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    positioneditdialog.cpp \
    registration.cpp \
    security.cpp \
    shifttemplatedialog.cpp

HEADERS += \
    addemployeedialog.h \
    addshiftdialog.h \
    businesslist.h \
    businessmainwindow.h \
    databasemanager.h \
    employeecardwindow.h \
    login.h \
    mainwindow.h \
    positioneditdialog.h \
    registration.h \
    security.h \
    shifttemplatedialog.h

FORMS += \
    businesslist.ui \
    businessmainwindow.ui \
    login.ui \
    registration.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
