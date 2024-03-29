#ifndef CQDataFrameFile_H
#define CQDataFrameFile_H

#ifdef FILE_DATA

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>

//class CQEdit;
class CQViApp;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(File)

// file
class FileWidget : public Widget {
  Q_OBJECT

 public:
  static void s_init();

 public:
  FileWidget(Area *area, const QString &fileName="");

  void addWidgets() override;

  QString id() const override;

  const QString &fileName() const { return fileName_; }

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  QString  fileName_;
//CQEdit*  edit_ { nullptr };
  CQViApp* edit_ { nullptr };
};

class FileFactory : public WidgetFactory {
 public:
  FileFactory() { }

  const char *name() const override { return "file"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("file", new FileTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<FileWidget>(area);
  }
};

}

#endif

#endif
