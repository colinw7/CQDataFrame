#include <CQDataFrameCanvas.h>

#include <CSVGUtil.h>

#include <QPainter>

namespace CQDataFrame {

CanvasWidget *s_canvas = nullptr;

CanvasWidget::
CanvasWidget(Area *area, int width, int height) :
 Widget(area)
{
  setObjectName("canvas");

  setContentsWidth (width );
  setContentsHeight(height);

  setWindowRange();
}

QString
CanvasWidget::
id() const
{
  return QString("canvas.%1").arg(pos());
}

void
CanvasWidget::
setContentsWidth(int w)
{
  Widget::setContentsWidth(w);

  updateSize();
}

void
CanvasWidget::
setContentsHeight(int h)
{
  Widget::setContentsHeight(h);

  updateSize();
}

bool
CanvasWidget::
getNameValue(const QString &name, QVariant &value) const
{
  if      (name == "xmin" || name == "ymin" || name == "xmax" || name == "ymax") {
    if      (name == "xmin") value = xmin();
    else if (name == "ymin") value = ymin();
    else if (name == "xmax") value = xmax();
    else if (name == "ymax") value = ymax();
  }
  else
    return Widget::getNameValue(name, value);

  return true;
}

bool
CanvasWidget::
setNameValue(const QString &name, const QVariant &value)
{
  bool ok = true;

  if      (name == "xmin" || name == "ymin" || name == "xmax" || name == "ymax") {
    if      (name == "xmin") setXMin(value.toDouble(&ok));
    else if (name == "ymin") setYMin(value.toDouble(&ok));
    else if (name == "xmax") setXMax(value.toDouble(&ok));
    else if (name == "ymax") setYMax(value.toDouble(&ok));

    if (ok)
      setWindowRange();
  }
  else
    return Widget::setNameValue(name, value);

  if (! ok)
    return false;

  return true;
}

void
CanvasWidget::
updateSize()
{
  int iw = contentsWidth ();
  int ih = contentsHeight();

  if (iw != image_.width() || ih != image_.height()) {
    image_ = QImage(iw, ih, QImage::Format_ARGB32);

    displayRange_.setPixelRange(0, 0, iw, ih);

    dirty_ = true;
  }
}

void
CanvasWidget::
setWindowRange()
{
  displayRange_.setWindowRange(xmin(), ymin(), xmax(), ymax());

  dirty_ = true;
}

void
CanvasWidget::
draw(QPainter *painter, int dx, int dy)
{
  if (image_.width() <= 0 || image_.height() <= 0)
    return;

  if (dirty_) {
    ipainter_ = new QPainter;

    ipainter_->begin(&image_);

    ipainter_->fillRect(QRect(0, 0, image_.width(), image_.height()), bgColor_);

    auto *frame = this->frame();

    s_canvas = this;

    ipainter_->setPen  (Qt::red);
    ipainter_->setBrush(Qt::green);

    if (drawProc_ != "") {
      auto *qtcl = frame->qtcl();

      QString cmd = QString("%1 %2").arg(drawProc_).arg(id());

      bool log = true;

      (void) qtcl->eval(cmd, /*showError*/true, /*showResult*/log);
    }

    s_canvas = nullptr;

    ipainter_->end();

    delete ipainter_;

    dirty_ = false;
  }

  painter->drawImage(dx, dy, image_);
}

QSize
CanvasWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
CanvasWidget::
contentsSize() const
{
  return QSize(width(), height());
}

void
CanvasWidget::
setBrush(const QBrush &brush)
{
  assert(ipainter_);

  ipainter_->setBrush(brush);
}

void
CanvasWidget::
setPen(const QPen &pen)
{
  assert(ipainter_);

  ipainter_->setPen(pen);
}

void
CanvasWidget::
drawPath(const QString &path)
{
  assert(ipainter_);

  //---

  class Visitor : public CSVGUtil::PathVisitor {
   public:
    Visitor(QPainter *painter, const DisplayRange &displayRange) :
     painter_(painter), displayRange_(displayRange) {
    }

    void moveTo(double x, double y) override {
      path_.moveTo(qpoint(CPoint2D(x, y)));
    }

    void lineTo(double x, double y) override {
      path_.lineTo(qpoint(CPoint2D(x, y)));
    }

    void arcTo(double rx, double ry, double xa, int fa, int fs, double x2, double y2) override {
      bool unit_circle = false;

      CSVGUtil::BezierList beziers;

      CSVGUtil::arcToBeziers(lastX(), lastY(), x2, y2, xa, rx, ry, fa, fs, unit_circle, beziers);

      if (! beziers.empty())
        path_.lineTo(qpoint(beziers[0].getFirstPoint()));

      for (const auto &bezier : beziers)
        path_.cubicTo(qpoint(bezier.getControlPoint1()),
                      qpoint(bezier.getControlPoint2()),
                      qpoint(bezier.getLastPoint    ()));
    }

    void bezier2To(double x1, double y1, double x2, double y2) override {
      path_.quadTo(qpoint(CPoint2D(x1, y1)), qpoint(CPoint2D(x2, y2)));
    }

    void bezier3To(double x1, double y1, double x2, double y2, double x3, double y3) override {
      path_.cubicTo(qpoint(CPoint2D(x1, y1)), qpoint(CPoint2D(x2, y2)), qpoint(CPoint2D(x3, y3)));
    }

    void closePath(bool /*relative*/) override {
      path_.closeSubpath();
    }

    void term() override {
      painter_->drawPath(path_);
    }

   private:
    QPointF qpoint(const CPoint2D &p) {
      double px, py;

      displayRange_.windowToPixel(p.x, p.y, &px, &py);

      return QPointF(px, py);
    }

   private:
    QPainter*    painter_ { nullptr };
    DisplayRange displayRange_;
    QPainterPath path_;
  };

  Visitor visitor(ipainter_, displayRange_);

  CSVGUtil::visitPath(path.toStdString(), visitor);
}

void
CanvasWidget::
drawPixel(const QPointF &p)
{
  assert(ipainter_);

  double px, py;

  if (isMapping()) {
    windowToPixel(p.x(), p.y(), &px, &py);
  }
  else {
    px = p.x();
    py = p.y();
  }

  ipainter_->drawPoint(int(px), int(py));
}

void
CanvasWidget::
windowToPixel(double wx, double wy, double *px, double *py) const
{
  displayRange_.windowToPixel(wx, wy, px, py);
}

void
CanvasWidget::
pixelToWindow(double px, double py, double *wx, double *wy) const
{
  displayRange_.pixelToWindow(px, py, wx, wy);
}

//------

void
CanvasTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-width" , ArgType::Integer, "width");
  addArg(argv, "-height", ArgType::Integer, "height");
  addArg(argv, "-proc"  , ArgType::String , "draw proc");
}

QStringList
CanvasTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
CanvasTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  int width  = -1;
  int height = -1;

  if (argv.hasParseArg("width"))
    width = argv.getParseInt("width");

  if (argv.hasParseArg("height"))
    height = argv.getParseInt("height");

  int minWidth  = 100;
  int minHeight = 100;

  width  = std::max(width , minWidth );
  height = std::max(height, minHeight);

  auto *area = frame_->larea();

  //---

  auto *canvasWidget = makeWidget<CanvasWidget>(area, width, height);

  auto *cmd = new CanvasInstTclCmd(frame_, canvasWidget->id());

  frame_->tclCmdMgr()->addCommand(canvasWidget->id(), cmd);

  //---

  if (argv.hasParseArg("proc"))
    canvasWidget->setDrawProc(argv.getParseStr("proc"));

  return frame_->setCmdRc(canvasWidget->id());
}

//------

void
CanvasInstTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-path"   , ArgType::String, "path");
  addArg(argv, "-pixel"  , ArgType::String, "pixel");

  addArg(argv, "-fill"   , ArgType::String, "fill");
  addArg(argv, "-stroke" , ArgType::String, "stroke");

  addArg(argv, "-mapping", ArgType::SBool , "mapping enabled");

  addArg(argv, "-pixel_to_window", ArgType::String , "pixel to window");
  addArg(argv, "-window_to_pixel", ArgType::String , "window to pixel");
}

QStringList
CanvasInstTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
CanvasInstTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto *canvas = s_canvas;
  if (! canvas) return false;

  //---

  if (argv.hasParseArg("fill")) {
    QBrush brush(Qt::SolidPattern);

    auto fill = argv.getParseStr("fill");

    QStringList fillStrs;

    CQTclUtil::splitList(fill, fillStrs);

    for (auto &str : fillStrs) {
      QStringList strs;

      CQTclUtil::splitList(str, strs);
      if (strs.length() != 2) continue;

      if      (strs[0] == "visible") {
        bool ok;

        if (! Frame::s_stringToBool(strs[1], &ok))
          brush.setStyle(Qt::NoBrush);
      }
      else if (strs[0] == "color") {
        brush.setColor(QColor(strs[1]));
      }
    }

    canvas->setBrush(brush);
  }

    //---

  if (argv.hasParseArg("stroke")) {
    QPen pen(Qt::SolidLine);

    auto stroke = argv.getParseStr("stroke");

    QStringList strokeStrs;

    CQTclUtil::splitList(stroke, strokeStrs);

    for (auto &str : strokeStrs) {
      QStringList strs;

      CQTclUtil::splitList(str, strs);
      if (strs.length() != 2) continue;

      if      (strs[0] == "visible") {
        bool ok;

        if (! Frame::s_stringToBool(strs[1], &ok))
          pen.setStyle(Qt::NoPen);
      }
      else if (strs[0] == "color") {
        pen.setColor(QColor(strs[1]));
      }
      else if (strs[0] == "width") {
        bool ok;

        pen.setWidthF(strs[1].toDouble(&ok));
      }
      else if (strs[0] == "cap") {
        if      (strs[1] == "round")
          pen.setCapStyle(Qt::RoundCap);
        else if (strs[1] == "square")
          pen.setCapStyle(Qt::SquareCap);
        else if (strs[1] == "flat")
          pen.setCapStyle(Qt::FlatCap);
      }
    }

    canvas->setPen(pen);
  }

  //---

  auto argToPoint = [&](const QString &arg, QPointF &p) {
    QStringList strs;

    CQTclUtil::splitList(arg, strs);

    if (strs.length() != 2)
      return false;

    bool ok;

    double x = strs[0].toDouble(&ok);
    if (! ok) return false;

    double y = strs[1].toDouble(&ok);
    if (! ok) return false;

    p = QPointF(x, y);

    return true;
  };

  //---

  if (argv.hasParseArg("mapping")) {
    auto str = argv.getParseStr("mapping");

    bool ok;

    if (! Frame::s_stringToBool(str, &ok))
      canvas->setMapping(false);
  }

  if      (argv.hasParseArg("path")) {
    auto path = argv.getParseStr("path");

    canvas->drawPath(path);
  }
  else if (argv.hasParseArg("pixel")) {
    auto arg = argv.getParseStr("pixel");

    QPointF point;

    if (! argToPoint(arg, point))
      return false;

    canvas->drawPixel(point);
  }
  else if (argv.hasParseArg("pixel_to_window")) {
    auto arg = argv.getParseStr("pixel_to_window");

    QPointF point;

    if (! argToPoint(arg, point))
      return false;

    double wx, wy;

    canvas->pixelToWindow(point.x(), point.y(), &wx, &wy);

    QVariantList vars;

    vars << QVariant(wx);
    vars << QVariant(wy);

    frame_->setCmdRc(vars);
  }
  else if (argv.hasParseArg("window_to_pixel")) {
    auto arg = argv.getParseStr("window_to_pixel");

    QPointF point;

    if (! argToPoint(arg, point))
      return false;

    double px, py;

    canvas->windowToPixel(point.x(), point.y(), &px, &py);

    QVariantList vars;

    vars << QVariant(px);
    vars << QVariant(py);

    frame_->setCmdRc(vars);
  }

  //---

  canvas->setMapping(true);

  return true;
}

}
