#ifndef CQDataFrameSVG_H
#define CQDataFrameSVG_H

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>
#include <CQDataFrameFileText.h>

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(SVG)

// svg
class SVGWidget : public Widget {
  Q_OBJECT

 public:
  SVGWidget(Area *area, const FileText &fileText=FileText());

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  FileText fileText_;
};

class SVGFactory : public WidgetFactory {
 public:
  SVGFactory() { }

  const char *name() const override { return "svg"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("svg", new SVGTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<SVGWidget>(area);
  }
};

}

#endif
