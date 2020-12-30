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
\
CQCsvModel.cpp \
CQDataModel.cpp \
CQBaseModel.cpp \
CQModelDetails.cpp \
CQModelVisitor.cpp \
CQModelUtil.cpp \
CQModelNameValues.cpp \
CQValueSet.cpp \
CQTrie.cpp \
\
CSVGUtil.cpp \
CTclUtil.cpp \
CFileMatch.cpp \

HEADERS += \
../include/CQDataFrame.h \
\
../include/CQCsvModel.h \
../include/CQDataModel.h \
../include/CQBaseModel.h \
../include/CQModelDetails.h \
../include/CQModelVisitor.h \
../include/CQModelUtil.h \
../include/CQModelNameValues.h \
../include/CQValueSet.h \
../include/CQTrie.h \
\
../include/CQBaseModelTypes.h \
../include/CQStatData.h \
\
../include/CSVGUtil.h \
../include/CQTclUtil.h \
../include/CTclUtil.h \
../include/CFileMatch.h \

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
