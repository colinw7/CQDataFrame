#include <CQDataFrameImage.h>

#include <QPainter>
#include <QMenu>

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
contentsWidth() const
{
  if (! simage_.isNull())
    return simage_.width();
  else
    return image_.width();
}

void
ImageWidget::
setContentsWidth(int w)
{
  if (image_.isNull())
    return;

  xscale_ = w/image_.width();

  Widget::setContentsWidth(w);

  updateImage();
}

int
ImageWidget::
contentsHeight() const
{
  if (! simage_.isNull())
    return simage_.height();
  else
    return image_.height();
}

void
ImageWidget::
setContentsHeight(int h)
{
  if (image_.isNull())
    return;

  yscale_ = h/image_.height();

  Widget::setContentsHeight(h);

  updateImage();
}

void
ImageWidget::
updateImage()
{
  if (image_.isNull())
    return;

  int iwidth  = image_.width ();
  int iheight = image_.height();

  if (xscale_ != 1.0 || yscale_ != 1.0) {
    int swidth  = int(xscale_*iwidth);
    int sheight = int(yscale_*iheight);

    simage_ = image_.scaled(swidth, sheight);
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
addMenuItems(QMenu *menu)
{
  Widget::addMenuItems(menu);

  auto *zoomInAction = menu->addAction("Zoom In");

  connect(zoomInAction, SIGNAL(triggered()), this, SLOT(zoomInSlot()));

  auto *zoomOutAction = menu->addAction("Zoom Out");

  connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOutSlot()));

  if (xscale_ != 1.0 || yscale_ != 1.0) {
    auto *resetAction = menu->addAction("Reset Size");

    connect(resetAction, SIGNAL(triggered()), this, SLOT(resetSlot()));
  }
}

void
ImageWidget::
zoomInSlot()
{
  xscale_ *= 2.0;
  yscale_ *= 2.0;

  updateImage();

  placeWidgets();
}

void
ImageWidget::
zoomOutSlot()
{
  xscale_ /= 2.0;
  yscale_ /= 2.0;

  updateImage();

  placeWidgets();
}

void
ImageWidget::
resetSlot()
{
  xscale_ = 1.0;
  yscale_ = 1.0;

  updateImage();

  placeWidgets();
}

void
ImageWidget::
draw(QPainter *painter, int dx, int dy)
{
  if (! simage_.isNull())
    painter->drawImage(dx, dy, simage_);
  else
    painter->drawImage(dx, dy, image_);
}

QSize
ImageWidget::
contentsSizeHint() const
{
  return image_.size();
}

QSize
ImageWidget::
contentsSize() const
{
  QSize s;

  if (! simage_.isNull())
    s = simage_.size();
  else
    s = image_.size();

  return QSize(s.width(), s.height());
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

  if (! argv.hasParseArg("file"))
    return false;

  auto fileName = argv.getParseStr("file");

  auto *area = frame_->larea();

  auto *widget = makeWidget<ImageWidget>(area, fileName);

  //---

  if (argv.hasParseArg("width")) {
    int width = argv.getParseInt("width");

    widget->setContentsWidth(width);
  }

  if (argv.hasParseArg("height")) {
    int height = argv.getParseInt("height");

    widget->setContentsHeight(height);
  }

  return frame_->setCmdRc(widget->id());
}

}
