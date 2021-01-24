#include <CQDataFrameImage.h>

#include <QPainter>

namespace CQDataFrame {

ImageWidget::
ImageWidget(Area *area, const QString &file) :
 Widget(area)
{
  setObjectName("image");

  setFile(file);
}

QString
ImageWidget::
id() const
{
  return QString("image.%1").arg(pos());
}

void
ImageWidget::
setFile(const QString &s)
{
  file_ = s;

  image_ = QImage(file_);

  updateImage();
}

int
ImageWidget::
width() const
{
  if (! simage_.isNull())
    return simage_.width();
  else
    return image_.width();
}

void
ImageWidget::
setWidth(int w)
{
  Widget::setWidth(w);

  updateImage();
}

int
ImageWidget::
height() const
{
  if (! simage_.isNull())
    return simage_.height();
  else
    return image_.height();
}

void
ImageWidget::
setHeight(int h)
{
  Widget::setHeight(h);

  updateImage();
}

void
ImageWidget::
updateImage()
{
  if (Widget::width() > 0 || Widget::height() > 0) {
    int width  = image_.width ();
    int height = image_.height();

    if (Widget::width()  > 0) width  = Widget::width();
    if (Widget::height() > 0) height = Widget::height();

    simage_ = image_.scaled(width, height);
  }
  else
    simage_ = QImage();

  update();
}

bool
ImageWidget::
getNameValue(const QString &name, QVariant &value) const
{
  if      (name == "file")
    value = file_;
  else if (name == "width")
    value = (! simage_.isNull() ? simage_.width() : image_.width());
  else if (name == "height")
    value = (! simage_.isNull() ? simage_.height() : image_.height());
  else
    return Widget::getNameValue(name, value);

  return true;
}

bool
ImageWidget::
setNameValue(const QString &name, const QVariant &value)
{
  if      (name == "file") {
    setFile(value.toString());
  }
  else
    return Widget::setNameValue(name, value);

  return true;
}

void
ImageWidget::
pasteText(const QString &text)
{
  setFile(text);
}

void
ImageWidget::
draw(QPainter *painter)
{
  const auto &margins = contentsMargins();

  int x = margins.left();
  int y = margins.top ();

  if (! simage_.isNull())
    painter->drawImage(x, y, simage_);
  else
    painter->drawImage(x, y, image_);
}

QSize
ImageWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  //---

  QSize s;

  if (! simage_.isNull())
    s = simage_.size();
  else
    s = image_.size();

  return QSize(s.width() + xm, s.height() + ym);
}

//------

void
ImageTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file"  , ArgType::String , "file name");
  addArg(argv, "-width" , ArgType::Integer, "width");
  addArg(argv, "-height", ArgType::Integer, "height");
}

QStringList
ImageTclCmd::
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
ImageTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto *area = frame_->larea();

  if (! argv.hasParseArg("file"))
    return false;

  auto fileName = argv.getParseStr("file");

  auto *widget = new ImageWidget(area, fileName);

  area->addWidget(widget);

  //---

  if (argv.hasParseArg("width")) {
    int width = argv.getParseInt("width");

    widget->setWidth(width);
  }

  if (argv.hasParseArg("height")) {
    int height = argv.getParseInt("height");

    widget->setHeight(height);
  }

  return frame_->setCmdRc(widget->id());
}

}
