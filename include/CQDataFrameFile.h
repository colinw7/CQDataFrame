#ifndef CQDataFrameFile_H
#define CQDataFrameFile_H

#ifdef FILE_DATA

#include <CQDataFrame.h>

//class CQEdit;
class CQVi;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(File)

// file
class FileWidget : public Widget {
  Q_OBJECT

 public:
  static void s_init();

 public:
  FileWidget(Area *area, const QString &fileName="");

  QString id() const override;

  const QString &fileName() const { return fileName_; }

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private:
  QString fileName_;
//CQEdit* edit_ { nullptr };
  CQVi*   edit_ { nullptr };
};

class FileFactory : public WidgetFactory {
 public:
  FileFactory() { }

  const char *name() const override { return "file"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("file", new FileTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    auto *widget = new FileWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif

#endif
