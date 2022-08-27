QT       += core gui charts concurrent sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas -Wunused-parameter -Wunused-function

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    clickableQLabel.cpp \
    datatypes.cpp \
    deletion_confirmation_dialog.cpp \
    dupe_results_dialog.cpp \
    exif_rename_builder_dialog.cpp \
    folder_list_item.cpp \
    gutils.cpp \
    main.cpp \
    mainwindow.cpp \
    metadata_selection_dialog.cpp \
    stats_dialog.cpp \
    ExifTool.cpp \
    ExifToolPipe.cpp \
    TagInfo.cpp

HEADERS += \
    clickableQLabel.h \
    datatypes.h \
    deletion_confirmation_dialog.h \
    dupe_results_dialog.h \
    exif_rename_builder_dialog.h \
    folder_list_item.h \
    gutils.h \
    mainwindow.h \
    meta_converters.h \
    metadata_selection_dialog.h \
    stats_dialog.h \
    ExifTool.h \
    ExifToolPipe.h \
    TagInfo.h

FORMS += \
    deletion_confirmation_dialog.ui \
    dupe_results_dialog.ui \
    exif_rename_builder_dialog.ui \
    folder_list_item.ui \
    mainwindow.ui \
    metadata_selection_dialog.ui \
    stats_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
