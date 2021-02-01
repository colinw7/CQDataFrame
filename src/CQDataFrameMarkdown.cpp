#include <CQDataFrameMarkdown.h>

#ifdef MARKDOWN_DATA

#include <CMarkdown.h>

#include <QTextEdit>
#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>

namespace CQDataFrame {

MarkdownWidget::
MarkdownWidget(Area *area, const FileText &fileText) :
 Widget(area), fileText_(fileText)
{
  setObjectName("markdown");
}

void
MarkdownWidget::
addWidgets()
{
  auto *layout = new QVBoxLayout(contents_);
  layout->setMargin(0); layout->setSpacing(0);

  textEdit_ = new QTextEdit;

  textEdit_->setObjectName("edit");
  textEdit_->setReadOnly(true);

  layout->addWidget(textEdit_);

  setFileText(fileText_);
}

QString
MarkdownWidget::
id() const
{
  return QString("markdown.%1").arg(pos());
}

void
MarkdownWidget::
setFileText(const FileText &fileText)
{
  fileText_ = fileText;

  CMarkdown markdown;

  QString htmlText;

  if (fileText.type == FileText::Type::FILENAME) {
    QStringList lines;

    if (! Frame::s_fileToLines(fileText.value, lines))
      return;

    htmlText = markdown.textToHtml(lines.join("\n"));
  }
  else {
    htmlText = markdown.textToHtml(fileText_.value);
  }

  textEdit_->setHtml(htmlText);
}

void
MarkdownWidget::
setExpanded(bool b)
{
  textEdit_->setVisible(b);

  Widget::setExpanded(b);
}

void
MarkdownWidget::
draw(QPainter *, int /*dx*/, int /*dy*/)
{
}

QSize
MarkdownWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
MarkdownWidget::
contentsSize() const
{
  auto *doc    = textEdit_->document();
  auto *layout = doc->documentLayout();

  int h = layout->documentSize().height() + 8;

  return QSize(-1, h);
}

//------

void
MarkdownTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file", ArgType::String, "markdown file");
  addArg(argv, "-text", ArgType::String, "markdown text");
}

QStringList
MarkdownTclCmd::
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return Frame::s_completeFile(file);
  }

  return strs;
}

bool
MarkdownTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  MarkdownWidget *widget = nullptr;

  auto *area = frame_->larea();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = makeWidget<MarkdownWidget>(area, FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = makeWidget<MarkdownWidget>(area, FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame_->setCmdRc(widget->id());
}

}

#endif
