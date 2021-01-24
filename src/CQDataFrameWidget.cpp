#include <CQDataFrameWidget.h>
#include <CQDataFrame.h>

#include <CQUtil.h>

#include <QApplication>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QPainter>

namespace CQDataFrame {

Widget::
Widget(Area *area) :
 QFrame(nullptr), area_(area)
{
  setObjectName("widget");

  bgColor_ = QColor(220, 220, 220);

  setFocusPolicy(Qt::NoFocus);

  setMouseTracking(true);

  setContextMenuPolicy(Qt::DefaultContextMenu);
}

void
Widget::
setExpanded(bool b)
{
  if (b != expanded_) {
    expanded_ = b;

    area()->placeWidgets();
  }
}

void
Widget::
setDocked(bool b)
{
  if (b != docked_) {
    docked_ = b;

    if (docked_) {
      larea()->removeWidget(this);

      rarea()->addWidget(this);
    }
    else {
      rarea()->removeWidget(this);

      larea()->addWidget(this);
    }
  }
}

void
Widget::
setFixedFont()
{
  auto fixedFont = CQUtil::getMonospaceFont();

  setFont(fixedFont);

  updateFont();
}

void
Widget::
contextMenuEvent(QContextMenuEvent *e)
{
  auto *menu = new QMenu;

  //---

  addMenuItems(menu);

  //---

  (void) menu->exec(e->globalPos());

  delete menu;
}

Area *
Widget::
larea() const
{
  auto *frame = this->frame();

  return frame->larea();
}

Area *
Widget::
rarea() const
{
  auto *frame = this->frame();

  return frame->rarea();
}

Frame *
Widget::
frame() const
{
  return area()->frame();
}

CQTcl *
Widget::
qtcl() const
{
  return frame()->qtcl();
}

bool
Widget::
getNameValue(const QString &name, QVariant &value) const
{
  if      (name == "x"     ) value = x     ();
  else if (name == "y"     ) value = y     ();
  else if (name == "width" ) value = width ();
  else if (name == "height") value = height();
  else
    return false;

  return true;
}

bool
Widget::
setNameValue(const QString &name, const QVariant &value)
{
  bool ok { true };

  if      (name == "width")
    setWidth(value.toInt(&ok));
  else if (name == "height")
    setHeight(value.toInt(&ok));
  else
    return false;

  if (! ok)
    return false;

  return true;
}

void
Widget::
addMenuItems(QMenu *menu)
{
  auto *copyAction  = menu->addAction("&Copy\tCtrl+C");
  auto *pasteAction = menu->addAction("&Paste\tCtrl+V");

  connect(copyAction, SIGNAL(triggered()), this, SLOT(copySlot()));
  connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteSlot()));

  //---

  if (canCollapse()) {
    auto *expandAction = menu->addAction("Expanded");

    expandAction->setCheckable(true);
    expandAction->setChecked  (isExpanded());

    connect(expandAction, SIGNAL(triggered(bool)), this, SLOT(setExpanded(bool)));
  }

  if (canDock()) {
    auto *dockAction = menu->addAction("Dock");

    dockAction->setCheckable(true);
    dockAction->setChecked  (isDocked());

    connect(dockAction, SIGNAL(triggered(bool)), this, SLOT(setDocked(bool)));
  }

  if (canClose()) {
    auto *closeAction = menu->addAction("Close");

    connect(closeAction, SIGNAL(triggered()), this, SLOT(closeSlot()));
  }

  menu->addSeparator();

  auto *loadAction = menu->addAction("Load");

  connect(loadAction, SIGNAL(triggered()), this, SLOT(loadSlot()));

  auto *saveAction = menu->addAction("Save");

  connect(saveAction, SIGNAL(triggered()), this, SLOT(saveSlot()));
}

void
Widget::
mousePressEvent(QMouseEvent *e)
{
  if      (e->button() == Qt::LeftButton) {
    const auto &margins = contentsMargins();

    if (e->x() < margins.left() || e->x() >= width () - margins.right () - 1 ||
        e->y() < margins.top () || e->y() >= height() - margins.bottom() - 1) {
      if (e->x() >= width () - margins.right () - 1 &&
          e->y() >= height() - margins.bottom() - 1) {
        mouseData_.dragging = true;
        mouseData_.resizing = true;
      }
      else {
        if (isDocked())
          mouseData_.dragging = true;
      }

      if (mouseData_.dragging) {
        auto pos = mapToGlobal(e->pos());

        mouseData_.dragX  = this->x();
        mouseData_.dragY  = this->y();
        mouseData_.dragW  = this->width ();
        mouseData_.dragH  = this->height();
        mouseData_.dragX1 = pos.x();
        mouseData_.dragY1 = pos.y();

        return;
      }
    }

    pixelToText(e->pos(), mouseData_.pressLineNum, mouseData_.pressCharNum);

    mouseData_.pressed     = true;
    mouseData_.moveLineNum = mouseData_.pressLineNum;
    mouseData_.moveCharNum = mouseData_.pressCharNum;
  }
  else if (e->button() == Qt::MiddleButton) {
    paste();

    update();
  }
}

void
Widget::
mouseMoveEvent(QMouseEvent *e)
{
  if      (mouseData_.dragging) {
    auto pos = mapToGlobal(e->pos());

    mouseData_.dragX2 = pos.x();
    mouseData_.dragY2 = pos.y();

    int dx = mouseData_.dragX2 - mouseData_.dragX1;
    int dy = mouseData_.dragY2 - mouseData_.dragY1;

    if (! mouseData_.resizing) {
      this->setX(mouseData_.dragX + dx);
      this->setY(mouseData_.dragY + dy);

      int xo = area()->xOffset();
      int yo = area()->yOffset();

      this->move(this->x() + xo, this->y() + yo);
    }
    else {
      this->setWidth (mouseData_.dragW + dx);
      this->setHeight(mouseData_.dragH + dy);

      this->resize(this->width(), this->height());

      if (! isDocked())
        area()->placeWidgets();
    }
  }
  else if (mouseData_.pressed) {
    pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

    update();
  }
  else {
    const auto &margins = contentsMargins();

    if      (e->x() >= width () - margins.right () - 1 &&
             e->y() >= height() - margins.bottom() - 1) {
      setCursor(Qt::SizeFDiagCursor);
    }
    else if (e->x() < margins.left() || e->x() >= width () - margins.right () - 1 ||
             e->y() < margins.top () || e->y() >= height() - margins.bottom() - 1) {
      if (isDocked())
        setCursor(Qt::SizeAllCursor);
      else
        setCursor(Qt::ArrowCursor);
    }
    else {
      setCursor(Qt::ArrowCursor);
    }
  }
}

void
Widget::
mouseReleaseEvent(QMouseEvent *e)
{
  if (mouseData_.pressed)
    pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

  mouseData_.reset();
}

void
Widget::
pixelToText(const QPoint &p, int &lineNum, int &charNum)
{
  // continuation lines
  int numLines = lines_.size();

  lineNum = -1;
  charNum = -1;

  int i { 0 }, y1 { 0 }, y2 { 0 };

  for ( ; i < numLines; ++i) {
    auto *line = lines_[i];

    y1 = line->y(); // top
    y2 = y1 + charData_.height - 1;

    if (p.y() >= y1 && p.y() <= y2) {
      lineNum = i;
      charNum = (p.x() - line->x())/charData_.width;
      return;
    }
  }
}

void
Widget::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  drawBorder(&painter);

  if (isExpanded()) {
    painter.fillRect(contentsRect(), bgColor_);

    draw(&painter);
  }
  else
    painter.fillRect(contentsRect(), QColor(200, 200, 200));
}

void
Widget::
resizeEvent(QResizeEvent *)
{
  // actual size
  width_  = QFrame::width ();
  height_ = QFrame::height();

  handleResize(width_, height_);
}

void
Widget::
drawBorder(QPainter *painter) const
{
  QColor titleColor(200, 200, 200);

  const auto &margins = contentsMargins();

  int l = margins.left  ();
  int t = margins.top   ();
  int r = margins.right ();
  int b = margins.bottom();

  int w = width ();
  int h = height();

  painter->fillRect(QRect(0, 0, w, t), titleColor);
  painter->fillRect(QRect(0, 0, l, h), titleColor);

  painter->fillRect(QRect(0, h - b, w, b), titleColor);
  painter->fillRect(QRect(w - r, 0, r, h), titleColor);
}

QRect
Widget::
contentsRect() const
{
  const auto &margins = contentsMargins();

  int l = margins.left  ();
  int t = margins.top   ();
  int r = margins.right ();
  int b = margins.bottom();

  int w = width ();
  int h = height();

  return QRect(l, t, w - l - r, h - t - b);
}

void
Widget::
drawText(QPainter *painter, int x, int y, const QString &text)
{
  int x1 = x;

  for (int i = 0; i < text.length(); ++i) {
    if (text[i] == '\n') {
      y += charData_.height;

      x1 = x;
    }
    else {
      painter->drawText(x1, y + charData_.ascent, text[i]);

      x1 += charData_.width;
    }
  }
}

void
Widget::
updateFont()
{
  QFontMetrics fm(font());

  charData_.width   = fm.width("X");
  charData_.height  = fm.height();
  charData_.ascent  = fm.ascent();
//charData_.descent = fm.descent();
}

void
Widget::
copySlot()
{
  copy();
}

void
Widget::
copyText(const QString &text) const
{
  auto *clipboard = QApplication::clipboard();

  clipboard->setText(text, QClipboard::Selection);
  clipboard->setText(text, QClipboard::Clipboard);
}

void
Widget::
pasteSlot()
{
  paste();
}

void
Widget::
paste()
{
  auto *clipboard = QApplication::clipboard();

  auto text = clipboard->text(QClipboard::Selection);

  pasteText(text);
}

void
Widget::
closeSlot()
{
  area_->removeWidget(this);

  deleteLater();
}

void
Widget::
loadSlot()
{
  this->frame()->load();
}

void
Widget::
saveSlot()
{
  this->frame()->save();
}

QSize
Widget::
sizeHint() const
{
  if (! expanded_)
    return QSize(-1, 20);

  return calcSize();
}

}
