#ifndef CQDataFrameMarkdown_H
#define CQDataFrameMarkdown_H

#ifdef MARKDOWN_DATA

#include <CQDataFrame.h>
#include <CQDataFrameFileText.h>

class QTextEdit;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Markdown)

// markdown
class MarkdownWidget : public Widget {
  Q_OBJECT

 public:
  MarkdownWidget(Area *area, const FileText &fileText=FileText());

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

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
    auto *widget = new MarkdownWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif

#endif
