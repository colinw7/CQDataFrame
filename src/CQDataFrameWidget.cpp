#include <CQDataFrameWidget.h>
#include <CQDataFrame.h>
#include <CQDataFrameEscapeParse.h>

#include <CQUtil.h>
#include <CQStrParse.h>
#include <CCommand.h>
#include <COSExec.h>
#include <CEnv.h>

#include <QApplication>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QPainter>
#include <QHBoxLayout>

namespace CQDataFrame {

Widget::
Widget(Area *area) :
 QFrame(nullptr), area_(area)
{
  setObjectName("widget");

  setFocusPolicy(Qt::NoFocus);

  setMouseTracking(true);

  //---

  auto *layout = new QHBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  contents_ = new WidgetContents(this);

  contents_->setCursor(Qt::ArrowCursor);

  scrollArea_ = new CQScrollArea(contents_);

  scrollArea_->setCursor(Qt::ArrowCursor);

  connect(scrollArea_, SIGNAL(updateArea()), this, SLOT(contentsUpdateSlot()));

  layout->addWidget(scrollArea_);

  bgColor_ = QColor(220, 220, 220);

  setContextMenuPolicy(Qt::DefaultContextMenu);
}

void
Widget::
init()
{
  addWidgets();
}

void
Widget::
contentsUpdateSlot()
{
  contents_->update();
}

void
Widget::
setExpanded(bool b)
{
  if (b != expanded_) {
    expanded_ = b;

    placeWidgets();
  }
}

void
Widget::
setEditing(bool b)
{
  if (b != editing_) {
    editing_ = b;

    placeWidgets();
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

      x_ = rarea()->margin(); y_ = rarea()->margin();

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
setResizeFrame(bool b)
{
  if (b != resizeFrame_) {
    resizeFrame_ = b;
  }
}

void
Widget::
setResizeContents(bool b)
{
  if (b != resizeContents_) {
    resizeContents_ = b;
  }
}

void
Widget::
placeWidgets()
{
  area()->placeWidgets();
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

void
Widget::
setWidgetWidth(int w)
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right();

  setContentsWidth(w - xm);
}

void
Widget::
setWidgetHeight(int h)
{
  const auto &margins = contentsMargins();

  int ym = margins.top() + margins.bottom();

  setContentsHeight(h - ym);
}

void
Widget::
setContentsWidth(int w)
{
  width_ = w;

  updateSize();
}

void
Widget::
setContentsHeight(int h)
{
  height_ = h;

  updateSize();
}

bool
Widget::
getNameValue(const QString &name, QVariant &value) const
{
  if      (name == "x"     ) value = x             ();
  else if (name == "y"     ) value = y             ();
  else if (name == "width" ) value = contentsWidth ();
  else if (name == "height") value = contentsHeight();
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
    setContentsWidth(value.toInt(&ok));
  else if (name == "height")
    setContentsHeight(value.toInt(&ok));
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

  if (canCollapse() || canDock() || canClose())
  menu->addSeparator();

  if (canCollapse()) {
    auto *expandAction = menu->addAction("Expanded");

    expandAction->setCheckable(true);
    expandAction->setChecked  (isExpanded());

    connect(expandAction, SIGNAL(triggered(bool)), this, SLOT(setExpandedSlot(bool)));
  }

  if (canDock()) {
    auto *dockAction = menu->addAction("Dock");

    dockAction->setCheckable(true);
    dockAction->setChecked  (isDocked());

    connect(dockAction, SIGNAL(triggered(bool)), this, SLOT(setDocked(bool)));
  }

  if (canEdit()) {
    auto *editAction = menu->addAction("Edit");

    editAction->setCheckable(true);
    editAction->setChecked  (isEditing());

    connect(editAction, SIGNAL(triggered(bool)), this, SLOT(setEditing(bool)));
  }

  if (canClose()) {
    auto *closeAction = menu->addAction("Close");

    connect(closeAction, SIGNAL(triggered()), this, SLOT(closeSlot()));
  }

  //---

#if 0
  auto *resizeFrameAction = menu->addAction("Resize Frame");

  resizeFrameAction->setCheckable(true);
  resizeFrameAction->setChecked  (isResizeFrame());

  connect(resizeFrameAction, SIGNAL(triggered(bool)), this, SLOT(setResizeFrame(bool)));

  auto *resizeContentsAction = menu->addAction("Resize Contents");

  resizeContentsAction->setCheckable(true);
  resizeContentsAction->setChecked  (isResizeContents());

  connect(resizeContentsAction, SIGNAL(triggered(bool)), this, SLOT(setResizeContents(bool)));
#endif

  //---

  menu->addSeparator();

  auto *loadAction = menu->addAction("Load");

  connect(loadAction, SIGNAL(triggered()), this, SLOT(loadSlot()));

  auto *saveAction = menu->addAction("Save");

  connect(saveAction, SIGNAL(triggered()), this, SLOT(saveSlot()));
}

//---

void
Widget::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  // draw border (contents margins)
  drawBorder(&painter);

  //---

  // fill contents
  const auto &margins = contentsMargins();

  int l = margins.left  ();
  int t = margins.top   ();
  int r = margins.right ();
  int b = margins.bottom();

  int w = QFrame::width ();
  int h = QFrame::height();

  QRect rect(l, t, w - l - r, h - t - b);

  if (isExpanded())
    painter.fillRect(rect, bgColor_);
  else
    painter.fillRect(rect, QColor(200, 200, 200));
}

void
Widget::
mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton) {
    const auto &margins = contentsMargins();

    if (e->x() < margins.left() || e->x() >= width () - margins.right () - 1 ||
        e->y() < margins.top () || e->y() >= height() - margins.bottom() - 1) {
      // bottom right
      if (e->x() >= width () - margins.right () - 1 &&
          e->y() >= height() - margins.bottom() - 1) {
        mouseData_.dragging = true;
        mouseData_.resizing = true;
      }
      else if (e->x() >= width () - margins.right () - 1) {
        mouseData_.dragging = true;
        mouseData_.resizing = true;
        mouseData_.resizeY  = false;
      }
      else if (e->y() >= height() - margins.bottom() - 1) {
        mouseData_.dragging = true;
        mouseData_.resizing = true;
        mouseData_.resizeX  = false;
      }
      else {
        if (isDocked()) {
          mouseData_.dragging = true;

          raise();
        }
      }

      if (mouseData_.dragging) {
        auto pos = mapToGlobal(e->pos());

        mouseData_.dragX  = this->x();
        mouseData_.dragY  = this->y();
        mouseData_.dragW  = this->width();
        mouseData_.dragH  = this->height();
        mouseData_.dragX1 = pos.x();
        mouseData_.dragY1 = pos.y();

        return;
      }
    }
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
#if 0
      if (isResizeContents()) {
        if (mouseData_.resizeX)
          this->setContentsWidth(mouseData_.dragW + dx);
        else
          this->setContentsWidth(mouseData_.dragW);

        if (mouseData_.resizeY)
          this->setContentsHeight(mouseData_.dragH + dy);
        else
          this->setContentsHeight(mouseData_.dragH);
      }
#endif

      if (isResizeFrame()) {
        //const auto &margins = contentsMargins();

        //int xm = margins.left() + margins.right ();
        //int ym = margins.top () + margins.bottom();

        //int w = this->contentsWidth () + xm;
        //int h = this->contentsHeight() + ym;

        int w = mouseData_.dragW + dx;
        int h = mouseData_.dragH + dy;

        this->resize(w, h);
      }

      updateSize();

      placeWidgets();
    }
  }
  else {
    const auto &margins = contentsMargins();

    if      (e->x() >= width () - margins.right () - 1 &&
             e->y() >= height() - margins.bottom() - 1) {
      setCursor(Qt::SizeFDiagCursor);
    }
    else if (e->x() >= width () - margins.right () - 1) {
      setCursor(Qt::SizeHorCursor);
    }
    else if (e->y() >= height() - margins.bottom() - 1) {
      setCursor(Qt::SizeVerCursor);
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
mouseReleaseEvent(QMouseEvent *)
{
  mouseData_.reset();
}

//---

void
Widget::
contentsMousePress(QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton) {
    (void) pixelToText(e->pos(), mouseData_.pressLineNum, mouseData_.pressCharNum);

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
contentsMouseMove(QMouseEvent *e)
{
  if (mouseData_.pressed) {
    (void) pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

    update();
  }
}

void
Widget::
contentsMouseRelease(QMouseEvent *e)
{
  if (mouseData_.pressed)
    (void) pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

  mouseData_.reset();
}

//---

bool
Widget::
pixelToText(const QPoint &p, int &lineNum, int &charNum)
{
  // continuation lines
  int numLines = int(lines_.size());

  lineNum = -1;
  charNum = -1;

  int i { 0 }, y1 { 0 }, y2 { 0 };

  for ( ; i < numLines; ++i) {
    auto *line = lines_[size_t(i)];

    y1 = line->y(); // top
    y2 = y1 + charData_.height - 1;

    if (p.y() >= y1 && p.y() <= y2) {
      lineNum = i;
      charNum = (p.x() - line->x())/charData_.width;

      return true;
    }
  }

  return false;
}

void
Widget::
updateSize()
{
  auto size = contents_->contentsSize();

  scrollArea_->setXSize(size.width ());
  scrollArea_->setYSize(size.height());

  scrollArea_->showHBar(QFrame::width () < size.width ());
  scrollArea_->showVBar(QFrame::height() < size.height());
}

void
Widget::
drawContents(QPainter *painter)
{
  if (isExpanded()) {
    int dx = scrollArea_->getXOffset();
    int dy = scrollArea_->getYOffset();

    draw(painter, dx, dy);
  }
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
  int w = width ();
  int h = height();

  return QRect(0, 0, w, h);
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

  charData_.width   = fm.horizontalAdvance("X");
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

//---

bool
Widget::
runTclCommand(const QString &line, QString &res) const
{
  auto *frame = this->frame();

  auto *qtcl = frame->qtcl();

  COSExec::grabOutput();

  bool log = true;

  bool tclRc = qtcl->eval(line, /*showError*/true, /*showResult*/log);

  std::cout << std::flush;

  std::string res1;

  COSExec::readGrabbedOutput(res1);

  COSExec::ungrabOutput();

  res = res1.c_str();

  return tclRc;
}

//---

bool
Widget::
runUnixCommand(const std::string &cmd, const Args &args, QString &res) const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right();

  //---

  // set terminal columns
  int numCols = (width() - xm)/charData_.width;

  CEnvInst.set("COLUMNS", numCols);

  // run command
  CCommand command(cmd, cmd, args);

  std::string res1;

  command.addStringDest(res1);

  command.start();

  command.wait();

  int cmdRc = command.getReturnCode();

  if (cmdRc != 0)
    return false;

  //---

#ifdef ESCAPE_PARSER
  EscapeParse eparse;

  res1 = eparse.processString(res1);
#endif

  res = QString(res1.c_str());

  return true;
}

//---

void
Widget::
parseCommand(const QString &line, std::string &name, std::vector<std::string> &args) const
{
  assert(line.length());

  CQStrParse parse(line);

  parse.skipSpace();

  int pos = parse.getPos();

  parse.skipNonSpace();

  name = parse.getBefore(pos).toStdString();

  while (! parse.eof()) {
    parse.skipSpace();

    int pos = parse.getPos();

    parse.skipNonSpace();

    auto arg = parse.getBefore(pos);

    args.push_back(arg.toStdString());
  }
}

//---

QSize
Widget::
sizeHint() const
{
  QSize s = contents_->sizeHint();

  const auto &margins = contentsMargins();

  int w = s.width ();
  int h = s.height();

  if (w > 0) { int xm = margins.left() + margins.right (); w += xm; }
  if (h > 0) { int ym = margins.top () + margins.bottom(); h += ym; }

  return QSize(w, h);
}

//---

WidgetContents::
WidgetContents(Widget *widget) :
 widget_(widget)
{
  setObjectName("contents");

  setFocusPolicy(Qt::NoFocus);

  setMouseTracking(true);
}

void
WidgetContents::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  widget_->drawContents(&painter);
}

void
WidgetContents::
resizeEvent(QResizeEvent *)
{
  //widget_->resizeContents(width(), height());
}

void
WidgetContents::
mousePressEvent(QMouseEvent *e)
{
  widget_->contentsMousePress(e);
}

void
WidgetContents::
mouseMoveEvent(QMouseEvent *e)
{
  widget_->contentsMouseMove(e);
}

void
WidgetContents::
mouseReleaseEvent(QMouseEvent *e)
{
  widget_->contentsMouseRelease(e);
}

QSize
WidgetContents::
sizeHint() const
{
  if (! widget_->isExpanded())
    return QSize(-1, 20);

  return widget_->contentsSizeHint();
}

QSize
WidgetContents::
contentsSize() const
{
  if (! widget_->isExpanded())
    return QSize(-1, 20);

  return widget_->contentsSize();
}

}
