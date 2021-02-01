#include <CQDataFrameHtml.h>

#include <QTextEdit>
#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>

namespace CQDataFrame {

HtmlWidget::
HtmlWidget(Area *area, const FileText &fileText) :
 Widget(area), fileText_(fileText)
{
  setObjectName("html");
}

void
HtmlWidget::
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
HtmlWidget::
id() const
{
  return QString("html.%1").arg(pos());
}

void
HtmlWidget::
setFileText(const FileText &fileText)
{
  fileText_ = fileText;

  QString htmlText;

  if (fileText.type == FileText::Type::FILENAME) {
    QStringList lines;

    if (! Frame::s_fileToLines(fileText.value, lines))
      return;

    htmlText = lines.join("\n");
  }
  else {
    htmlText = fileText_.value;
  }

  textEdit_->setHtml(htmlText);
}

void
HtmlWidget::
setExpanded(bool b)
{
  textEdit_->setVisible(b);

  Widget::setExpanded(b);
}

void
HtmlWidget::
draw(QPainter *, int /*dx*/, int /*dy*/)
{
}

QSize
HtmlWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
HtmlWidget::
contentsSize() const
{
  return QSize(width(), height());
}

//------

void
HtmlTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file", ArgType::String, "html file");
  addArg(argv, "-text", ArgType::String, "html text");
}

QStringList
HtmlTclCmd::
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
HtmlTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  HtmlWidget *widget = nullptr;

  auto *area = frame_->larea();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = makeWidget<HtmlWidget>(area, FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = makeWidget<HtmlWidget>(area, FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame_->setCmdRc(widget->id());
}

//------

}
