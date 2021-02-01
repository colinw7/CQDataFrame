#ifndef CQDataFrameMarkdown_H
#define CQDataFrameMarkdown_H

#ifdef MARKDOWN_DATA

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>
#include <CQDataFrameFileText.h>

class QTextEdit;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Markdown)

// markdown
class MarkdownWidget : public Widget {
  Q_OBJECT

 public:
  MarkdownWidget(Area *area, const FileText &fileText=FileText());

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

class MarkdownFactory : public WidgetFactory {
 public:
  MarkdownFactory() { }

  const char *name() const override { return "markdown"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("markdown", new MarkdownTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<MarkdownWidget>(area);
  }
};

}

#endif

#endif
