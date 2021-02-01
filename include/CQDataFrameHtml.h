#ifndef CQDataFrameHtml_H
#define CQDataFrameHtml_H

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>
#include <CQDataFrameFileText.h>

class QTextEdit;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Html)

// html text
class HtmlWidget : public Widget {
  Q_OBJECT

 public:
  HtmlWidget(Area *area, const FileText &fileText=FileText());

  void addWidgets() override;

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  void setExpanded(bool b) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  FileText   fileText_;
  QTextEdit* textEdit_ { nullptr };
};

//---

class HtmlFactory : public WidgetFactory {
 public:
  HtmlFactory() { }

  const char *name() const override { return "html"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("html", new HtmlTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<HtmlWidget>(area);
  }
};

}

#endif
