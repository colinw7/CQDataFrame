TEMPLATE = app

TARGET = CQDataFrameTest

QT += widgets svg webkitwidgets

DEPENDPATH += .

QMAKE_CXXFLAGS += -std=c++14

MOC_DIR = .moc

SOURCES += \
CQDataFrameTest.cpp \

HEADERS += \
CQDataFrameTest.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CUtil/include \
../../CMath/include \
../../COS/include \
/usr/include/tcl \

unix:LIBS += \
-L../lib \
-L../../CQMarkdown/lib \
-L../../CQUtil/lib \
-L../../CCommand/lib \
-L../../CImageLib/lib \
-L../../CFont/lib \
-L../../CConfig/lib \
-L../../CFileUtil/lib \
-L../../CFile/lib \
-L../../CUtil/lib \
-L../../CMath/lib \
-L../../CRegExp/lib \
-L../../CGlob/lib \
-L../../CStrUtil/lib \
-L../../COS/lib \
-lCQDataFrame \
-lCQMarkdown \
-lCQUtil \
-lCCommand \
-lCImageLib \
-lCFont \
-lCConfig \
-lCFileUtil \
-lCFile \
-lCUtil \
-lCMath \
-lCRegExp \
-lCGlob \
-lCStrUtil \
-lCOS \
-ljpeg -lpng -ltcl -ltre -lcurses
