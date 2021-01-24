#include <CQDataFrameHtml.h>

#include <QTextEdit>
#include <QVBoxLayout>
#include <QAbstractTextDocumentLayout>

namespace CQDataFrame {

HtmlWidget::
HtmlWidget(Area *area, const FileText &fileText) :
 Widget(area)
{
  setObjectName("html");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  textEdit_ = new QTextEdit;

  textEdit_->setObjectName("edit");
  textEdit_->setReadOnly(true);

  layout->addWidget(textEdit_);

  setFileText(fileText);
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
draw(QPainter *)
{
}

QSize
HtmlWidget::
calcSize() const
{
  auto *doc    = textEdit_->document();
  auto *layout = doc->documentLayout();

  int h = layout->documentSize().height() + 8;

  return QSize(-1, h);
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

    widget = new HtmlWidget(area, FileText(FileText::Type::FILENAME, file));

    area->addWidget(widget);
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = new HtmlWidget(area, FileText(FileText::Type::TEXT, text));

    area->addWidget(widget);
  }
  else
    return false;

  return frame_->setCmdRc(widget->id());
}

//------

}
