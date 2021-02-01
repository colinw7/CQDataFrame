TEMPLATE = lib

TARGET = CQDataFrame

QT += widgets svg webkitwidgets

DEPENDPATH += .

QMAKE_CXXFLAGS += \
-std=c++14 \

MOC_DIR = .moc

CONFIG += staticlib
CONFIG += c++14

SOURCES += \
CQDataFrame.cpp \
CQDataFrameCanvas.cpp \
CQDataFrameCommand.cpp \
CQDataFrameFile.cpp \
CQDataFrameFileMgr.cpp \
CQDataFrameHistory.cpp \
CQDataFrameHtml.cpp \
CQDataFrameImage.cpp \
CQDataFrameMarkdown.cpp \
CQDataFrameSVG.cpp \
CQDataFrameTclCmd.cpp \
CQDataFrameTcl.cpp \
CQDataFrameText.cpp \
CQDataFrameUnix.cpp \
CQDataFrameWeb.cpp \
CQDataFrameWidget.cpp \
\
CSVGUtil.cpp \
CQTclCmd.cpp \
CTclUtil.cpp \
CTclParse.cpp \

HEADERS += \
../include/CQDataFrame.h \
../include/CQDataFrameCanvas.h \
../include/CQDataFrameCommand.h \
../include/CQDataFrameEscapeParse.h \
../include/CQDataFrameFile.h \
../include/CQDataFrameFileMgr.h \
../include/CQDataFrameFileText.h \
../include/CQDataFrameHistory.h \
../include/CQDataFrameHtml.h \
../include/CQDataFrameImage.h \
../include/CQDataFrameMarkdown.h \
../include/CQDataFrameSVG.h \
../include/CQDataFrameTclCmd.h \
../include/CQDataFrameTcl.h \
../include/CQDataFrameText.h \
../include/CQDataFrameUnix.h \
../include/CQDataFrameWeb.h \
../include/CQDataFrameWidget.h \
\
../include/CSVGUtil.h \
../include/CQTclCmd.h \
../include/CQTclUtil.h \
../include/CTclUtil.h \

DESTDIR     = ../lib
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
. \
../include \
../../CCsv/include \
../../CQUtil/include \
../../CQMarkdown/include \
../../CCommand/include \
../../CImageLib/include \
../../CFont/include \
../../CUtil/include \
../../CFile/include \
../../CMath/include \
../../CGlob/include \
../../CStrUtil/include \
../../COS/include \
/usr/include/tcl \
