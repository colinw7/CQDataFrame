#ifndef CQDataFrameFileMgr_H
#define CQDataFrameFileMgr_H

#ifdef FILE_MGR_DATA

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>

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

  void addWidgets() override;

  QString id() const override;

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private Q_SLOTS:
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
    return makeWidget<FileMgrWidget>(area);
  }
};

}

#endif

#endif
