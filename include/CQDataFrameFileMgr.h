#ifndef CQDataFrameFileMgr_H
#define CQDataFrameFileMgr_H

#ifdef FILE_MGR_DATA

#include <CQDataFrame.h>

class CQFileBrowser;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(FileMgr)

// filemgr
class FileMgrWidget : public Widget {
  Q_OBJECT

 public:
  static void s_init();

 public:
  FileMgrWidget(Area *area);

  QString id() const override;

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private slots:
  void fileActivated(const QString &filename);

 private:
  CQFileBrowser *fileMgr_ { nullptr };
};

class FileMgrFactory : public WidgetFactory {
 public:
  FileMgrFactory() { }

  const char *name() const override { return "filemgr"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("filemgr", new FileMgrTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    auto *widget = new FileMgrWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif

#endif
