#include <CQDataFrameSVG.h>

#include <QSvgRenderer>

#include <QVBoxLayout>

namespace CQDataFrame {

SVGWidget::
SVGWidget(Area *area, const FileText &fileText) :
 Widget(area), fileText_(fileText)
{
  setObjectName("svg");
}

QString
SVGWidget::
id() const
{
  return QString("svg.%1").arg(pos());
}

bool
SVGWidget::
getNameValue(const QString &name, QVariant &value) const
{
  return Widget::getNameValue(name, value);
}

bool
SVGWidget::
setNameValue(const QString &name, const QVariant &value)
{
  return Widget::setNameValue(name, value);
}

void
SVGWidget::
draw(QPainter *painter, int /*dx*/, int /*dy*/)
{
  QSvgRenderer renderer;

  if (fileText_.type == FileText::Type::TEXT) {
    QByteArray ba(fileText_.value.toLatin1());

    renderer.load(ba);
  }
  else {
    renderer.load(fileText_.value);
  }

  renderer.render(painter, contentsRect());
}

QSize
SVGWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
SVGWidget::
contentsSize() const
{
  QSvgRenderer renderer;

  if (fileText_.type == FileText::Type::TEXT) {
    QByteArray ba(fileText_.value.toLatin1());

    renderer.load(ba);
  }
  else {
    renderer.load(fileText_.value);
  }

  auto s = renderer.defaultSize();

  int width  = s.width ();
  int height = s.height();

  if (this->width () > 0) width  = this->width ();
  if (this->height() > 0) height = this->height();

  return QSize(width, height);
}

//------

void
SVGTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file", ArgType::String, "svg file");
  addArg(argv, "-text", ArgType::String, "svg text");
}

QStringList
SVGTclCmd::
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
SVGTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  SVGWidget *widget = nullptr;

  auto *area = frame_->larea();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = makeWidget<SVGWidget>(area, FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = makeWidget<SVGWidget>(area, FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame_->setCmdRc(widget->id());
}

//------

}
