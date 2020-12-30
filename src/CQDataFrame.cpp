#include <CQDataFrame.h>
#include <CQTclUtil.h>
#include <CQTabSplit.h>
#include <CQUtil.h>
#include <CCommand.h>
#include <CMarkdown.h>
#include <CQCsvModel.h>
#include <CQModelDetails.h>
#include <CQModelUtil.h>
#include <CSVGUtil.h>
#include <CQStrParse.h>
#include <CFileMatch.h>
#include <CFile.h>
#include <CEnv.h>
#include <COSFile.h>
#include <COSExec.h>

#ifdef USE_WEBENGINE
#include <QWebEngineView>
#include <QWebEngineFrame>
#else
#include <QWebView>
#include <QWebFrame>
#endif

#include <QApplication>
#include <QTextEdit>
#include <QAbstractTextDocumentLayout>
#include <QSvgRenderer>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QPainter>
#include <QMenu>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QFile>
#include <QTextStream>

namespace CQDataFrame {

bool stringToBool(const QString &str, bool *ok) {
  QString lstr = str.toLower();

  if (lstr == "0" || lstr == "false" || lstr == "no") {
    *ok = true;
    return false;
  }

  if (lstr == "1" || lstr == "true" || lstr == "yes") {
    *ok = true;
    return true;
  }

  *ok = false;

  return false;
}

bool fileToLines(const QString &fileName, QStringList &lines)
{
  QFile file(fileName);

  if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  QString line;

  while (! file.atEnd()) {
    QByteArray bytes = file.readLine();

    QString line1(bytes);

    if (line1.right(1) == '\n')
      line1 = line1.mid(0, line1.length() - 1);

    lines.push_back(line1);
  }

  return true;
}

//---

class TclCmd {
 public:
  using Vars = std::vector<QVariant>;

 public:
  TclCmd(Frame *frame, const QString &name) :
   frame_(frame), name_(name) {
    cmdId_ = frame_->qtcl()->createObjCommand(name_,
      (CQTcl::ObjCmdProc) &TclCmd::commandProc, (CQTcl::ObjCmdData) this);
  }

  static int commandProc(ClientData clientData, Tcl_Interp *, int objc, const Tcl_Obj **objv) {
    auto *command = (TclCmd *) clientData;

    Vars vars;

    for (int i = 1; i < objc; ++i) {
      auto *obj = const_cast<Tcl_Obj *>(objv[i]);

      vars.push_back(command->frame_->qtcl()->variantFromObj(obj));
    }

    if (! command->frame_->processTclCmd(command->name_, vars))
      return TCL_ERROR;

    return TCL_OK;
  }

 private:
  Frame*      frame_ { nullptr };
  QString     name_;
  Tcl_Command cmdId_ { nullptr };
};

//---

Frame::
Frame(QWidget *parent) :
 QFrame(parent)
{
  setObjectName("dataFrame");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  // tab for sequential and docked widgets
  tab_ = new CQTabSplit;

  layout->addWidget(tab_);

  // scrolled widget and command widget
  lscroll_ = new Scroll(this, /*isCommand*/true);

  tab_->addWidget(lscroll_, "History");

  // scrolled docked widgets
  rscroll_ = new Scroll(this, /*isCommand*/false);

  tab_->addWidget(rscroll_, "Docked");

  //--

  status_ = new Status(this);

  layout->addWidget(status_);

  //---

  qtcl_ = new CQTcl;

  qtcl_->createAlias("echo", "puts");

  addTclCommand("image"   , new TclImageCmd   (this));
  addTclCommand("canvas"  , new TclCanvasCmd  (this));
  addTclCommand("web"     , new TclWebCmd     (this));
  addTclCommand("markdown", new TclMarkdownCmd(this));
  addTclCommand("html"    , new TclHtmlCmd    (this));
  addTclCommand("svg"     , new TclSVGCmd     (this));
  addTclCommand("model"   , new TclModelCmd   (this));

  addTclCommand("get_data", new TclGetDataCmd(this));
  addTclCommand("set_data", new TclSetDataCmd(this));

  addTclCommand("get_model_data", new TclGetModelDataCmd(this));
  addTclCommand("show_model"    , new TclShowModelCmd   (this));

}

Frame::
~Frame()
{
  delete qtcl_;
}

//---

void
Frame::
addTclCommand(const QString &name, TclCmdProc *proc)
{
  proc->setName(name);

  auto *tclCmd = new TclCmd(this, name);

  proc->setTclCmd(tclCmd);

  commandNames_.push_back(name);

  commandProcs_[name] = proc;
}

bool
Frame::
processTclCmd(const QString &name, const Vars &vars)
{
  auto p = commandProcs_.find(name);

  if (p != commandProcs_.end()) {
    auto *proc = (*p).second;

    TclCmdArgs argv(name, vars);

    return proc->exec(argv);
  }

  //---

  return false;
}

//------

bool
Frame::
setCmdRc(int rc)
{
  qtcl()->setResult(rc);

  return true;
}

bool
Frame::
setCmdRc(double rc)
{
  qtcl()->setResult(rc);

  return true;
}

bool
Frame::
setCmdRc(const QString &rc)
{
  qtcl()->setResult(rc);

  return true;
}

bool
Frame::
setCmdRc(const QVariant &rc)
{
  qtcl()->setResult(rc);

  return true;
}

bool
Frame::
setCmdRc(const QStringList &rc)
{
  qtcl()->setResult(rc);

  return true;
}

//---

void
Frame::
save()
{
  auto dir = QDir::current().dirName();

  auto fileName = QFileDialog::getSaveFileName(this, "Save Frame", dir, "Files (*.frame)");
  if (! fileName.length()) return; // cancelled

  QFile file(fileName);

  if (! file.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream os(&file);

  lscroll_->area()->save(os);
  rscroll_->area()->save(os);
}

bool
Frame::
load()
{
  auto dir = QDir::current().dirName();

  auto fileName = QFileDialog::getOpenFileName(this, "Open File", dir, "Files (*.frame)");
  if (! fileName.length()) return false; // cancelled

  return load(fileName);
}

bool
Frame::
load(const QString &fileName)
{
  QStringList lines;

  if (! fileToLines(fileName, lines))
    return false;

  auto *area = lscroll()->area();

  auto *commandWidget = area->commandWidget();
  assert(commandWidget);

  QString line;

  for (const auto &line1 : lines) {
    if (line1.trimmed() == "")
      continue;

    if (line.length())
      line += "\n" + line1;
    else
      line = line1;

    bool isTcl { false };

    if (commandWidget->isCompleteLine(line, isTcl)) {
      commandWidget->processCommand(line);

      line = "";
    }
  }

  if (line != "")
    commandWidget->processCommand(line);

  area->moveToEnd(commandWidget);

  return true;
}

QString
Frame::
addModel(CQDataModel *model)
{
  auto id = QString("model.%1").arg(namedModel_.size() + 1);

  assert(namedModel_.find(id) == namedModel_.end());

  ModelData modelData;

  modelData.id      = id;
  modelData.model   = model;
  modelData.details = new CQModelDetails(modelData.model);

  namedModel_[id] = modelData;

  return id;
}

CQDataModel *
Frame::
getModel(const QString &id) const
{
  auto p = namedModel_.find(id);
  if (p == namedModel_.end()) return nullptr;

  return (*p).second.model;
}

CQModelDetails *
Frame::
getModelDetails(const QString &id) const
{
  auto p = namedModel_.find(id);
  if (p == namedModel_.end()) return nullptr;

  return (*p).second.details;
}

//---

Scroll::
Scroll(Frame *frame, bool isCommand) :
 CQScrollArea(frame, nullptr), frame_(frame), isCommand_(isCommand)
{
  setObjectName("dataScroll");

  area_ = new Area(this);

  setWidget(area_);
}

void
Scroll::
updateContents()
{
  area()->update();

  area()->placeWidgets();
}

//---

Area::
Area(Scroll *scroll) :
 scroll_(scroll)
{
  setObjectName("area");

  // single command entry widget
  if (scroll_->isCommand())
    command_ = addCommandWidget();
}

Area::
~Area()
{
}

void
Area::
setPrompt(const QString &s)
{
  prompt_ = s;
}

CommandWidget *
Area::
commandWidget() const
{
  if (command_)
    return command_;

  return scroll()->frame()->lscroll()->area()->commandWidget();
}

void
Area::
save(QTextStream &os)
{
  for (const auto &widget : widgets_)
    widget->save(os);
}

//---

CommandWidget *
Area::
addCommandWidget()
{
  auto *widget = new CommandWidget(this);

  addWidget(widget);

  return widget;
}

TextWidget *
Area::
addTextWidget(const QString &text)
{
  auto *widget = new TextWidget(this, text);

  addWidget(widget);

  return widget;
}

UnixWidget *
Area::
addUnixWidget(const QString &cmd, const Args &args, const QString &res)
{
  auto *widget = new UnixWidget(this, cmd, args, res);

  addWidget(widget);

  return widget;
}

TclWidget *
Area::
addTclWidget(const QString &line, const QString &res)
{
  auto *widget = new TclWidget(this, line, res);

  addWidget(widget);

  return widget;
}

HistoryWidget *
Area::
addHistoryWidget(const QString &text)
{
  auto *widget = new HistoryWidget(this, ++ind_, text);

  addWidget(widget);

  return widget;
}

ImageWidget *
Area::
addImageWidget(const QString &name)
{
  auto *widget = new ImageWidget(this, name);

  addWidget(widget);

  return widget;
}

CanvasWidget *
Area::
addCanvasWidget(int width, int height)
{
  auto *widget = new CanvasWidget(this, width, height);

  addWidget(widget);

  auto *frame = scroll()->frame();

  auto *cmd = new TclCanvasInstCmd(frame, widget->id());

  frame->addTclCommand(widget->id(), cmd);

  return widget;
}

WebWidget *
Area::
addWebWidget(const QString &addr)
{
  auto *widget = new WebWidget(this, addr);

  addWidget(widget);

  return widget;
}

MarkdownWidget *
Area::
addMarkdownWidget(const FileText &fileText)
{
  auto *widget = new MarkdownWidget(this, fileText);

  addWidget(widget);

  return widget;
}

HtmlWidget *
Area::
addHtmlWidget(const FileText &fileText)
{
  auto *widget = new HtmlWidget(this, fileText);

  addWidget(widget);

  return widget;
}

SVGWidget *
Area::
addSVGWidget(const FileText &fileText)
{
  auto *widget = new SVGWidget(this, fileText);

  addWidget(widget);

  return widget;
}

void
Area::
addWidget(Widget *widget)
{
  widget->setParent(this);
  widget->setVisible(true);

  if (widget->pos() < 0) {
    widgets_.push_back(widget);

    widget->setPos(widgets_.size() - 1);
  }
  else {
    bool added = false;

    Widgets widgets;

    for (const auto &widget1 : widgets_) {
      if (! added && widget1->pos() > widget->pos()) {
        widgets.push_back(widget);

        added = true;
      }

      widgets.push_back(widget1);
    }

    if (! added)
      widgets.push_back(widget);

    std::swap(widgets_, widgets);
  }

  connect(widget, SIGNAL(contentsChanged()), this, SLOT(updateWidgets()));

  if (scroll_->isCommand())
    widget->setContentsMargins(margin_, margin_, margin_, margin_);
  else
    widget->setContentsMargins(margin_, margin_ + 16, margin_, margin_);

  placeWidgets();
}

void
Area::
removeWidget(Widget *widget)
{
  disconnect(widget, SIGNAL(contentsChanged()), this, SLOT(updateWidgets()));

  Widgets widgets;

  for (const auto &widget1 : widgets_)
    if (widget1 != widget)
      widgets.push_back(widget1);

  std::swap(widgets_, widgets);

  widget->setParent(nullptr);

  placeWidgets();
}

Widget *
Area::
getWidget(const QString &id) const
{
  for (const auto &widget : widgets_)
    if (widget->id() == id)
      return widget;

  return nullptr;
}

void
Area::
updateWidgets()
{
  scroll_->updateContents();
}

void
Area::
moveToEnd(Widget *widget)
{
  Widgets widgets;

  for (const auto &widget1 : widgets_)
    if (widget1 != widget)
      widgets.push_back(widget1);

  widgets.push_back(widget);

  widget->setPos(999999);

  std::swap(widgets_, widgets);

  placeWidgets();
}

int
Area::
xOffset() const
{
  return scroll()->getXOffset();
}

int
Area::
yOffset() const
{
  return scroll()->getYOffset();
}

void
Area::
resizeEvent(QResizeEvent *)
{
  placeWidgets();
}

bool
Area::
event(QEvent *e)
{
  if (e->type() == QEvent::Wheel) {
    scroll()->handleWheelEvent(static_cast<QWheelEvent *>(e));
  }

  return QFrame::event(e);
}

void
Area::
placeWidgets()
{
  int xo = this->xOffset();
  int yo = this->yOffset();

  int maxWidth = 0;

  if (scroll_->isCommand()) {
    int x = margin_;
    int y = margin_;

    for (const auto &widget : widgets_) {
      auto size = widget->sizeHint();

      int w1 = size.width (); if (w1 <= 0) w1 = width() - 2*margin_;
      int h1 = size.height(); if (h1 <= 0) h1 = 100;

      maxWidth = std::max(maxWidth, w1 + 2*margin_);

      widget->move  (x + xo, y + yo);
      widget->resize(w1, h1);

      y += h1 + margin_;
    }

    int maxHeight = y + 2*margin_;

    scroll()->setXSize(maxWidth );
    scroll()->setYSize(maxHeight);

    //---

    QFontMetrics fm(font());

    int charWidth  = fm.width("X");
    int charHeight = fm.height();

    scroll()->setXSingleStep(charWidth);
    scroll()->setYSingleStep(charHeight);
  }
  else {
    int maxHeight = 0;

    for (const auto &widget : widgets_) {
      auto size = widget->sizeHint();

      int x1 = widget->x() + xo;
      int y1 = widget->y() + yo;
      int w1 = size.width (); if (w1 <= 0) w1 = 100;
      int h1 = size.height(); if (h1 <= 0) h1 = 100;

      maxWidth  = std::max(maxWidth , x1 + w1);
      maxHeight = std::max(maxHeight, y1 + h1);

      widget->move(x1, y1);

      widget->resize(w1, h1);
    }

    scroll()->setXSize(maxWidth );
    scroll()->setYSize(maxHeight);
  }

  //---

  QFontMetrics fm(font());

  int charWidth  = fm.width("X");
  int charHeight = fm.height();

  scroll()->setXSingleStep(charWidth);
  scroll()->setYSingleStep(charHeight);
}

//---

Widget::
Widget(Area *area) :
 QFrame(nullptr), area_(area)
{
  setObjectName("widget");

  bgColor_ = QColor(220, 220, 220);

  setFocusPolicy(Qt::NoFocus);

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

    auto *frame = area()->scroll()->frame();

    auto *lscroll = frame->lscroll();
    auto *rscroll = frame->rscroll();

    if (docked_) {
      lscroll->area()->removeWidget(this);

      area_ = rscroll->area();

      rscroll->area()->addWidget(this);
    }
    else {
      rscroll->area()->removeWidget(this);

      area_ = lscroll->area();

      lscroll->area()->addWidget(this);
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

    if (e->x() < margins.left() || e->y() < margins.top()) {
      if (! area()->scroll()->isCommand()) {
        auto pos = mapToGlobal(e->pos());

        mouseData_.dragging = true;
        mouseData_.dragX    = this->x();
        mouseData_.dragY    = this->y();
        mouseData_.dragX1   = pos.x();
        mouseData_.dragY1   = pos.y();
      }
    }
    else {
      pixelToText(e->pos(), mouseData_.pressLineNum, mouseData_.pressCharNum);

      mouseData_.pressed     = true;
      mouseData_.moveLineNum = mouseData_.pressLineNum;
      mouseData_.moveCharNum = mouseData_.pressCharNum;
    }
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

    this->setX(mouseData_.dragX + dx);
    this->setY(mouseData_.dragY + dy);

    int xo = area()->xOffset();
    int yo = area()->yOffset();

    this->move(this->x() + xo, this->y() + yo);
  }
  else if (mouseData_.pressed) {
    pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

    update();
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
  handleResize(width(), height());
}

void
Widget::
drawBorder(QPainter *painter) const
{
  QColor titleColor(200, 200, 200);

  const auto &margins = contentsMargins();

  int l = margins.left  ();
  int t = margins.top   ();
  int r = margins.right  ();
  int b = margins.bottom();

  int w = width ();
  int h = height();

  painter->fillRect(QRect(0, 0, w, t), titleColor);
  painter->fillRect(QRect(0, 0, l, h), titleColor);

  painter->fillRect(QRect(0, h - 1 - b, w, b), titleColor);
  painter->fillRect(QRect(w - 1 - r, 0, r, h), titleColor);
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
  // TODO
}

void
Widget::
loadSlot()
{
  area()->scroll()->frame()->load();
}

void
Widget::
saveSlot()
{
  area()->scroll()->frame()->save();
}

QSize
Widget::
sizeHint() const
{
  if (! expanded_)
    return QSize(-1, 16);

  return calcSize();
}

//---

TextWidget::
TextWidget(Area *area, const QString &text) :
 Widget(area), text_(text)
{
  setObjectName("text");

  bgColor_ = QColor(204, 221, 255);

  //---

  setFixedFont();

  updateLines();
}

QString
TextWidget::
id() const
{
  return QString("text.%1").arg(pos());
}

void
TextWidget::
setText(const QString &text)
{
  text_ = text;

  updateLines();
}

void
TextWidget::
updateLines()
{
  for (auto &line : lines_)
    delete line;

  lines_.clear();

  //---

  const auto &text = this->text();

  int len = text.length();

  QString s;

  for (int i = 0; i < len; ++i) {
    if (text[i] == '\n') {
      lines_.push_back(new Line(s));

      s = "";
    }
    else
      s += text[i];
  }

  if (s.length())
    lines_.push_back(new Line(s));
}

void
TextWidget::
draw(QPainter *painter)
{
  const auto &margins = contentsMargins();

  int x = margins.left();
  int y = margins.top ();

  //---

  if (isError())
    painter->fillRect(contentsRect(), errorColor_);

  //---

  // draw lines
  painter->setPen(fgColor_);

  for (const auto &line : lines_) {
    line->setX(x);
    line->setY(y);

    drawText(painter, x, y, line->text());

    y += charData_.height;
  }

  //---

  // draw selection
  int lineNum1 = mouseData_.pressLineNum;
  int charNum1 = mouseData_.pressCharNum;
  int lineNum2 = mouseData_.moveLineNum;
  int charNum2 = mouseData_.moveCharNum;

  if (lineNum1 > lineNum2 || (lineNum1 == lineNum2 && charNum1 > charNum2)) {
    std::swap(lineNum1, lineNum2);
    std::swap(charNum1, charNum2);
  }

  if (lineNum1 != lineNum2 || charNum1 != charNum2)
    drawSelectedChars(painter, lineNum1, charNum1, lineNum2, charNum2);
}

void
TextWidget::
drawSelectedChars(QPainter *painter, int lineNum1, int charNum1, int lineNum2, int charNum2)
{
  int numLines = lines_.size();

  painter->setPen(bgColor_);

  for (int i = lineNum1; i <= lineNum2; ++i) {
    if (i < 0 || i >= numLines)
      continue;

    auto *line = lines_[i];

    int ty = line->y();
    int tx = line->x();

    const auto &text = line->text();

    int len = text.size();

    for (int j = 0; j < len; ++j) {
      if      (i == lineNum1 && i == lineNum2) {
        if (j < charNum1 || j > charNum2)
          continue;
      }
      else if (i == lineNum1) {
        if (j < charNum1)
          continue;
      }
      else if (i == lineNum2) {
        if (j > charNum2)
          continue;
      }

      int tx1 = tx + j*charData_.width;

      painter->fillRect(QRect(tx1, ty, charData_.width, charData_.height), selColor_);

      drawText(painter, tx1, ty, text.mid(j, 1));
    }
  }
}

void
TextWidget::
copy()
{
  copyText(selectedText());
}

QString
TextWidget::
selectedText() const
{
  int lineNum1 = mouseData_.pressLineNum;
  int charNum1 = mouseData_.pressCharNum;
  int lineNum2 = mouseData_.moveLineNum;
  int charNum2 = mouseData_.moveCharNum;

  if (lineNum1 > lineNum2 || (lineNum1 == lineNum2 && charNum1 > charNum2)) {
    std::swap(lineNum1, lineNum2);
    std::swap(charNum1, charNum2);
  }

  if (lineNum1 == lineNum2 && charNum1 == charNum2)
    return "";

  int numLines = lines_.size();

  QString str;

  for (int i = lineNum1; i <= lineNum2; ++i) {
    if (i < 0 || i >= numLines) continue;

    auto *line = lines_[i];

    const auto &text = line->text();

    if (str.length() > 0)
      str += "\n";

    int len = text.size();

    for (int j = 0; j < len; ++j) {
      if      (i == lineNum1 && i == lineNum2) {
        if (j < charNum1 || j > charNum2)
          continue;
      }
      else if (i == lineNum1) {
        if (j < charNum1)
          continue;
      }
      else if (i == lineNum2) {
        if (j > charNum2)
          continue;
      }

      str += text.mid(j, 1);
    }
  }

  return str;
}

QSize
TextWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  //---

  const auto &text = this->text();

  int len = text.length();

  int maxWidth     = 0;
  int currentWidth = 0;
  int numLines     = 0;

  for (int i = 0; i < len; ++i) {
    if (text[i] == '\n') {
      currentWidth = 0;

      ++numLines;
    }
    else {
      ++currentWidth;

      maxWidth = std::max(maxWidth, currentWidth);
    }
  }

  if (currentWidth > 0)
    ++numLines;

  return QSize(maxWidth*charData_.width + xm, numLines*charData_.height + ym);
}

//---

UnixWidget::
UnixWidget(Area *area, const QString &cmd, const Args &args, const QString &res) :
 TextWidget(area, res), cmd_(cmd), args_(args)
{
  setObjectName("unix");

  errMsg_ = "Error: command failed";
}

void
UnixWidget::
addMenuItems(QMenu *menu)
{
  TextWidget::addMenuItems(menu);

  auto *rerunAction = menu->addAction("Rerun");

  connect(rerunAction, SIGNAL(triggered()), this, SLOT(rerunSlot()));
}

void
UnixWidget::
rerunSlot()
{
  setIsError(false);

  auto *command = area_->commandWidget();

  std::string res;

  if (command->runUnixCommand(cmd_.toStdString(), args_, res))
    text_ = res.c_str();
  else
    setIsError(true);

  update();

  emit contentsChanged();
}

void
UnixWidget::
draw(QPainter *painter)
{
  if (! isError()) {
    TextWidget::draw(painter);
  }
  else {
    painter->fillRect(contentsRect(), errorColor_);

    const auto &margins = contentsMargins();

    int x = margins.left();
    int y = margins.top ();

    painter->setPen(fgColor_);

    drawText(painter, x, y, errMsg_);
  }
}

QSize
UnixWidget::
calcSize() const
{
  if (! isError()) {
    return TextWidget::calcSize();
  }
  else {
    const auto &margins = contentsMargins();

    int xm = margins.left() + margins.right ();
    int ym = margins.top () + margins.bottom();

    //---

    int len = errMsg_.length();

    return QSize(len*charData_.width + xm, charData_.height + ym);
  }
}

//---

TclWidget::
TclWidget(Area *area, const QString &line, const QString &res) :
 TextWidget(area, res), line_(line)
{
  setObjectName("tcl");
}

void
TclWidget::
addMenuItems(QMenu *menu)
{
  TextWidget::addMenuItems(menu);

  auto *rerunAction = menu->addAction("Rerun");

  connect(rerunAction, SIGNAL(triggered()), this, SLOT(rerunSlot()));
}

void
TclWidget::
rerunSlot()
{
  setIsError(false);

  auto *command = area_->commandWidget();

  QString res;

  if (command->runTclCommand(line_, res))
    text_ = res;
  else
    setIsError(true);

  update();

  emit contentsChanged();
}

void
TclWidget::
draw(QPainter *painter)
{
  TextWidget::draw(painter);
}

QSize
TclWidget::
calcSize() const
{
  return TextWidget::calcSize();
}

//---

HistoryWidget::
HistoryWidget(Area *area, int ind, const QString &text) :
 Widget(area), ind_(ind), text_(text)
{
  setObjectName("history");

  bgColor_ = QColor(230, 255, 238);

  //---

  setFixedFont();
}

QString
HistoryWidget::
id() const
{
  return QString("history.%1").arg(pos());
}

void
HistoryWidget::
draw(QPainter *painter)
{
  const auto &margins = contentsMargins();

  int x = margins.left();
  int y = margins.top ();

  painter->setPen(fgColor_);

  drawText(painter, x, y, text_);
}

void
HistoryWidget::
drawText(QPainter *painter, int x, int y, const QString &text)
{
  int x1 = x;

  auto indStr = QString("[%1] ").arg(ind_);

  painter->drawText(x1, y + charData_.ascent, indStr);

  x1 += charData_.width*indStr.length();

  for (int i = 0; i < text.length(); ++i) {
    if (text_[i] == '\n') {
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
HistoryWidget::
save(QTextStream &os)
{
  os << text_ << "\n";
}

QSize
HistoryWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  //---

  int len = text_.length();

  int numLines     = 0;
  int currentWidth = 0;

  auto indStr = QString("[%1] ").arg(ind_);

  currentWidth += indStr.length();

  int maxWidth = currentWidth;

  for (int i = 0; i < len; ++i) {
    if (text_[i] == '\n') {
      currentWidth = 0;

      ++numLines;
    }
    else {
      ++currentWidth;

      maxWidth = std::max(maxWidth, currentWidth);
    }
  }

  if (currentWidth > 0)
    ++numLines;

  return QSize(maxWidth*charData_.width + xm, numLines*charData_.height + ym);
}

//---

CommandWidget::
CommandWidget(Area *area) :
 Widget(area)
{
  setObjectName("command");

  //---

  setFocusPolicy(Qt::StrongFocus);

  //---

  setFixedFont();
}

QString
CommandWidget::
id() const
{
  return QString("command.%1").arg(pos());
}

void
CommandWidget::
draw(QPainter *painter)
{
  const auto &margins = contentsMargins();

  int x = margins.left();
  int y = margins.top ();

  //---

  // draw lines (above command line)
  int numLines = lines_.size();

  for (int i = 0; i < numLines; ++i) {
    auto *line = lines_[i];

    drawLine(painter, line, y);

    y += charData_.height;
  }

  //---

  // draw prompt on command line
  const auto &prompt = area()->prompt();

  promptY_     = y;
  promptWidth_ = prompt.length()*charData_.width;

  painter->setPen(fgColor_);

  drawText(painter, x, y, prompt);

  x += promptWidth_;

  //---

  // get entry text before/after cursor
  const auto &str = entry_.getText();
  int         pos = entry_.getPos();

  auto lhs = str.mid(0, pos);
  auto rhs = str.mid(pos);

  //---

  // draw entry text before and after cursor
  drawText(painter, x, y, lhs);

  x += lhs.length()*charData_.width;

  int cx = x;

  drawText(painter, x, y, rhs);

  x += rhs.length()*charData_.width;

  //---

  // draw cursor
  drawCursor(painter, cx, y, (rhs.length() ? rhs[0] : QChar()));

  // draw selection
  int lineNum1 = mouseData_.pressLineNum;
  int charNum1 = mouseData_.pressCharNum;
  int lineNum2 = mouseData_.moveLineNum;
  int charNum2 = mouseData_.moveCharNum;

  if (lineNum1 > lineNum2 || (lineNum1 == lineNum2 && charNum1 > charNum2)) {
    std::swap(lineNum1, lineNum2);
    std::swap(charNum1, charNum2);
  }

  if (lineNum1 != lineNum2 || charNum1 != charNum2)
    drawSelectedChars(painter, lineNum1, charNum1, lineNum2, charNum2);
}

void
CommandWidget::
drawCursor(QPainter *painter, int x, int y, const QChar &c)
{
  QRect r(x, y, charData_.width, charData_.height);

  if (hasFocus()) {
    painter->fillRect(r, cursorColor_);

    if (! c.isNull()) {
      painter->setPen(bgColor_);

      drawText(painter, x, y, QString(c));
    }
  }
  else {
    painter->setPen(cursorColor_);

    painter->drawRect(r);
  }
}

void
CommandWidget::
drawLine(QPainter *painter, Line *line, int y)
{
  const auto &margins = contentsMargins();

  int x = margins.left();

  //---

  line->setX(x);
  line->setY(y);

  QString text = line->text();

  if (line->isJoin())
    text += " \\";

  drawText(painter, x, y, text);
}

void
CommandWidget::
drawSelectedChars(QPainter *painter, int lineNum1, int charNum1, int lineNum2, int charNum2)
{
  const auto &margins = contentsMargins();

  int x = margins.left();

  //---

  int numLines = lines_.size();

  painter->setPen(bgColor_);

  for (int i = lineNum1; i <= lineNum2; ++i) {
    QString text;
    int     tx, ty;

    // continuation line
    if      (i >= 0 && i < numLines) {
      auto *line = lines_[i];

      ty = line->y();
      tx = line->x();

      text = line->text();
    }
    // current line
    else if (i == numLines) {
      ty = promptY_;
      tx = x;

      text = entry_.getText();
    }
    else
      continue;

    //---

    int len = text.size();

    for (int j = 0; j < len; ++j) {
      if      (i == lineNum1 && i == lineNum2) {
        if (j < charNum1 || j > charNum2)
          continue;
      }
      else if (i == lineNum1) {
        if (j < charNum1)
          continue;
      }
      else if (i == lineNum2) {
        if (j > charNum2)
          continue;
      }

      int tx1 = tx + j*charData_.width;

      painter->fillRect(QRect(tx1, ty, charData_.width, charData_.height), selColor_);

      drawText(painter, tx1, ty, text.mid(j, 1));
    }
  }
}

void
CommandWidget::
drawText(QPainter *painter, int x, int y, const QString &text)
{
  int len = text.length();

  for (int i = 0; i < len; ++i) {
    painter->drawText(x, y + charData_.ascent, text[i]);

    x += charData_.width;
  }
}

void
CommandWidget::
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

bool
CommandWidget::
complete(const QString &line, int pos, QString &newText, CompleteMode /*completeMode*/) const
{
  // parse command
  std::string              name;
  std::vector<std::string> args;

  parseCommand(line, name, args);

  if ("image") {
    std::string file = (! args.empty() ? args[0] : "");

    CFileMatch fileMatch;

    std::string file1 = fileMatch.mostMatchPrefix(file);

    if (file1.find(' ')) {
      std::string file2;

      uint len = file1.size();

      for (uint i = 0; i < len; ++i) {
        if (file1[i] == ' ')
          file2 += "\\";

        file2 += file1[i];
      }

      file1 = file2;
    }

    newText = QString(name.c_str()) + " " + file1.c_str();

    return true;
  }
  else {
    std::cerr << "Complete: '" << line.toStdString() << "' " << pos << "\n";
    return false;
  }
}

void
CommandWidget::
mousePressEvent(QMouseEvent *e)
{
  if      (e->button() == Qt::LeftButton) {
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
CommandWidget::
mouseMoveEvent(QMouseEvent *e)
{
  if (mouseData_.pressed) {
    pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

    update();
  }
}

void
CommandWidget::
mouseReleaseEvent(QMouseEvent *e)
{
  if (mouseData_.pressed)
    pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

  mouseData_.pressed = false;
}

void
CommandWidget::
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

  // current line
  const auto &margins = contentsMargins();

  int x = margins.left();

  y2 = y1 + charData_.height - 1;

  if (p.y() >= y1 && p.y() <= y2) {
    lineNum = i;
    charNum = (p.x() - x)/charData_.width;
    return;
  }
}

bool
CommandWidget::
event(QEvent *e)
{
  if (e->type() == QEvent::KeyPress) {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);

    if (ke->key() == Qt::Key_Tab) {
      CompleteMode completeMode = CompleteMode::Longest;

      if (ke->modifiers() & Qt::ControlModifier)
        completeMode = CompleteMode::Interactive;

      QString text;

      if (complete(getText(), entry_.getPos(), text, completeMode)) {
        entry_.setText(text);

        entry_.cursorEnd();

        update();
      }

      return true;
    }
  }

  return QFrame::event(e);
}

void
CommandWidget::
keyPressEvent(QKeyEvent *event)
{
  auto key = event->key();
  auto mod = event->modifiers();

  // run command
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    auto line = getText().trimmed();

    bool isTcl { false };

    bool isComplete = isCompleteLine(line, isTcl);

    if (mod & Qt::ControlModifier)
      isComplete = false;

    //---

    if (isComplete) {
      auto *lastLine = (! lines_.empty() ? lines_.back() : nullptr);

      if (lastLine && lastLine->isJoin()) {
        auto entryLine = entry_.getText();

        lastLine->setText(lastLine->text() + entryLine);

        lastLine->setJoin(false);

        clearEntry();
      }

      auto line = getText().trimmed();

      if (line != "") {
        processCommand(line);

        clearText();

        area()->moveToEnd(this);

        area()->scroll()->ensureVisible(0, area()->scroll()->getYSize());

        commands_.push_back(line);

        commandNum_ = -1;
      }
    }
    else {
      auto entryLine = entry_.getText();

      bool join = false;

      if (entryLine != "" && entryLine[entryLine.length() - 1] == '\\') {
        entryLine = entryLine.mid(0, entryLine.length() - 1);

        join = true;
      }

      auto *lastLine = (! lines_.empty() ? lines_.back() : nullptr);

      if (lastLine && lastLine->isJoin()) {
        lastLine->setText(lastLine->text() + entryLine);

        lastLine->setJoin(join);
      }
      else {
        if (! join)
          entryLine = entryLine.trimmed();

        auto *newLine = new Line(entryLine);

        newLine->setJoin(join);

        lines_.push_back(newLine);
      }

      clearEntry();

      area()->placeWidgets();
    }

    setFocus();
  }
  else if (key == Qt::Key_Left) {
    entry_.cursorLeft();
  }
  else if (key == Qt::Key_Right) {
    entry_.cursorRight();
  }
  else if (key == Qt::Key_Up) {
    QString command;

    int pos = commands_.size() + commandNum_;

    if (pos >= 0 && pos < commands_.size()) {
      command = commands_[pos];

      --commandNum_;
    }

    entry_.setText(command);

    entry_.cursorEnd();
  }
  else if (key == Qt::Key_Down) {
    ++commandNum_;

    QString command;

    int pos = commands_.size() + commandNum_;

    if (pos >= 0 && pos < commands_.size())
      command = commands_[pos];
    else
      commandNum_ = -1;

    entry_.setText(command);

    entry_.cursorEnd();
  }
  else if (key == Qt::Key_Backspace) {
    entry_.backSpace();
  }
  else if (key == Qt::Key_Backspace) {
    entry_.backSpace();
  }
  else if (key == Qt::Key_Delete) {
    entry_.deleteChar();
  }
  else if (key == Qt::Key_Insert) {
  }
  else if (key == Qt::Key_Tab) {
    // handled by event
  }
  else if (key == Qt::Key_Home) {
    // TODO
  }
  else if (key == Qt::Key_Escape) {
    // TODO
  }
  else if ((key >= Qt::Key_A && key <= Qt::Key_Z) && (mod & Qt::ControlModifier)) {
    if      (key == Qt::Key_A) // beginning
      entry_.cursorStart();
    else if (key == Qt::Key_E) // end
      entry_.cursorEnd();
//  else if (key == Qt::Key_U) // delete all before
//    entry_.clearBefore();
    else if (key == Qt::Key_H)
      entry_.backSpace();
//  else if (key == Qt::Key_W) // delete word before
//    entry_.clearWordBefore();
//  else if (key == Qt::Key_K) // delete all after
//    entry_.clearAfter();
//  else if (key == Qt::Key_T) // swap chars before
//    entry_.swapBefore();
    else if (key == Qt::Key_C)
      copy();
    else if (key == Qt::Key_V)
      paste();
  }
  else {
    entry_.insert(event->text());
  }

  update();
}

bool
CommandWidget::
isCompleteLine(const QString &line, bool &isTcl) const
{
  if (line == "") return false;

  // parse command
  std::string              name;
  std::vector<std::string> args;

  parseCommand(line, name, args);

  // TODO: command map
  if (name == "cd" || name[0] == '!') {
    isTcl = false;

    if (line[line.length() - 1] == '\\')
      return false;

    return true;
  }
  else {
    isTcl = true;

    return CTclUtil::isCompleteLine(line.toStdString());
  }
}

void
CommandWidget::
processCommand(const QString &line)
{
  // parse command
  std::string name;
  Args        args;

  parseCommand(line, name, args);

  if (name.empty()) return;

  area()->addHistoryWidget(line);

  // change directory
  if      (name == "cd") {
    std::string path;

    if (args.size() >= 1)
      path = args[0];
    else
      path = "~";

    std::string path1;

    if (CFile::expandTilde(path, path1))
      path = path1;

    if (COSFile::changeDir(path))
      area()->scroll()->frame()->status()->update();
    else {
      QString res = "Failed to change directory";

      auto *text = area()->addTextWidget(res);

      text->setIsError(true);
    }
  }
  // run unix command
  else if (name[0] == '!') {
    // run command
    auto cmd = name.substr(1);

    std::string res;

    bool rc = runUnixCommand(cmd, args, res);

    auto *unixCommand = area()->addUnixWidget(cmd.c_str(), args, res.c_str());

    if (! rc)
      unixCommand->setIsError(true);
  }
  // run tcl command
  else {
    QString res;

    bool rc = runTclCommand(line, res);

    if      (res.startsWith("<html>")) {
      area()->addHtmlWidget(FileText(FileText::Type::TEXT, res));
    }
    else if (res.startsWith("<svg>")) {
      area()->addSVGWidget(FileText(FileText::Type::TEXT, res));
    }
    else {
      auto *tclCommand = area()->addTclWidget(line, res);

      if (! rc)
        tclCommand->setIsError(true);
    }
  }
}

bool
CommandWidget::
runUnixCommand(const std::string &cmd, const Args &args, std::string &res) const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right();

  //---

  // set terminal columns
  int numCols = (width() - xm)/charData_.width;

  CEnvInst.set("COLUMNS", numCols);

  // run command
  CCommand command(cmd, cmd, args);

  command.addStringDest(res);

  command.start();

  command.wait();

  int cmdRc = command.getReturnCode();

  if (cmdRc != 0)
    return false;

  return true;
}

bool
CommandWidget::
runTclCommand(const QString &line, QString &res) const
{
  COSExec::grabOutput();

  bool log = true;

  auto *frame = area()->scroll()->frame();

  int tclRc = frame->qtcl()->eval(line, /*showError*/true, /*showResult*/log);

  std::cout << std::flush;

  std::string res1;

  COSExec::readGrabbedOutput(res1);

  COSExec::ungrabOutput();

  res = res1.c_str();

  if (tclRc != TCL_OK)
    return false;

  return true;
}

void
CommandWidget::
copy()
{
  copyText(selectedText());
}

void
CommandWidget::
pasteText(const QString &text)
{
  entry_.insert(text);
}

QString
CommandWidget::
selectedText() const
{
  int lineNum1 = mouseData_.pressLineNum;
  int charNum1 = mouseData_.pressCharNum;
  int lineNum2 = mouseData_.moveLineNum;
  int charNum2 = mouseData_.moveCharNum;

  if (lineNum1 > lineNum2 || (lineNum1 == lineNum2 && charNum1 > charNum2)) {
    std::swap(lineNum1, lineNum2);
    std::swap(charNum1, charNum2);
  }

  if (lineNum1 == lineNum2 && charNum1 == charNum2)
    return "";

  int numLines = lines_.size();

  QString str;

  for (int i = lineNum1; i <= lineNum2; ++i) {
     QString text;

    // continuation line
    if      (i >= 0 && i < numLines) {
      auto *line = lines_[i];

      text = line->text();
    }
    // current line
    else if (i == numLines) {
      text = entry_.getText();
    }
    else
      continue;

    //---

    if (str.length() > 0)
      str += "\n";

    int len = text.size();

    for (int j = 0; j < len; ++j) {
      if      (i == lineNum1 && i == lineNum2) {
        if (j < charNum1 || j > charNum2)
          continue;
      }
      else if (i == lineNum1) {
        if (j < charNum1)
          continue;
      }
      else if (i == lineNum2) {
        if (j > charNum2)
          continue;
      }

      str += text.mid(j, 1);
    }
  }

  return str;
}

QString
CommandWidget::
getText() const
{
  QString text;
  bool    first { true };

  int numLines = lines_.size();

  for (int i = 0; i < numLines; ++i) {
    auto *line = lines_[i];

    if (! first)
      text += "\n";

    text += line->text();

    first = false;
  }

  if (! first)
    text += "\n";

  text += entry_.getText();

  return text;
}

void
CommandWidget::
clearEntry()
{
  entry_.clear();

  mouseData_.reset();

  mouseData_.resetSelection();
}

void
CommandWidget::
clearText()
{
  clearEntry();

  lines_.clear();
}

QSize
CommandWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int ym = margins.top() + margins.bottom();

  //---

  int h = charData_.height;

  int numLines = lines_.size();

  h += numLines*charData_.height;

  return QSize(-1, h + ym);
}

//---

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

void
ImageWidget::
setWidth(int w)
{
  width_ = w;

  updateImage();
}

void
ImageWidget::
setHeight(int h)
{
  height_ = h;

  updateImage();
}

void
ImageWidget::
updateImage()
{
  if (width_ > 0 || height_ > 0) {
    int width  = image_.width ();
    int height = image_.height();

    if (width_  > 0) width  = width_;
    if (height_ > 0) height = height_;

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
    return false;

  return true;
}

bool
ImageWidget::
setNameValue(const QString &name, const QVariant &value)
{
  if      (name == "file") {
    setFile(value.toString());
  }
  else if (name == "width") {
    bool ok;

    setWidth(value.toInt(&ok));
  }
  else if (name == "height") {
    bool ok;

    setHeight(value.toInt(&ok));
  }
  else
    return false;

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

//---

CanvasWidget::
CanvasWidget(Area *area, int width, int height) :
 Widget(area), width_(width), height_(height)
{
  setObjectName("canvas");

  displayRange_.setWindowRange(0, 0, 100, 100);
}

QString
CanvasWidget::
id() const
{
  return QString("canvas.%1").arg(pos());
}

bool
CanvasWidget::
getNameValue(const QString &name, QVariant &value) const
{
  if      (name == "xmin" || name == "ymin" || name == "xmax" || name == "ymax") {
    double xmin, ymin, xmax, ymax;

    displayRange_.getWindowRange(&xmin, &ymin, &xmax, &ymax);

    if      (name == "xmin") value = xmin;
    else if (name == "ymin") value = ymin;
    else if (name == "xmax") value = xmax;
    else if (name == "ymax") value = ymax;
  }
  else if (name == "width")
    value = width_;
  else if (name == "height")
    value = height_;
  else
    return false;

  return true;
}

bool
CanvasWidget::
setNameValue(const QString &name, const QVariant &value)
{
  bool ok = true;

  if      (name == "xmin" || name == "ymin" || name == "xmax" || name == "ymax") {
    double xmin, ymin, xmax, ymax;

    displayRange_.getWindowRange(&xmin, &ymin, &xmax, &ymax);

    if      (name == "xmin") xmin = value.toDouble(&ok);
    else if (name == "ymin") ymin = value.toDouble(&ok);
    else if (name == "xmax") xmax = value.toDouble(&ok);
    else if (name == "ymax") ymax = value.toDouble(&ok);

    if (ok)
      displayRange_.setWindowRange(xmin, ymin, xmax, ymax);
  }
  else if (name == "width")
    width_ = value.toInt(&ok);
  else if (name == "height")
    height_ = value.toInt(&ok);
  else
    return false;

  if (! ok)
    return false;

  return true;
}

void
CanvasWidget::
handleResize(int w, int h)
{
  displayRange_.setPixelRange(0, 0, w - 1, h - 1);
}

void
CanvasWidget::
draw(QPainter *painter)
{
  auto *frame = area()->scroll()->frame();

  frame->setCanvas(this);

  painter_ = painter;

  painter_->setPen  (Qt::red);
  painter_->setBrush(Qt::green);

  if (drawProc_ != "") {
    QString cmd = QString("%1 %2").arg(drawProc_).arg(id());

    bool log = true;

    (void) frame->qtcl()->eval(cmd, /*showError*/true, /*showResult*/log);
  }

  frame->setCanvas(nullptr);
}

QSize
CanvasWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  //---

  return QSize(width_ + xm, height_ + ym);
}

void
CanvasWidget::
setBrush(const QBrush &brush)
{
  assert(painter_);

  painter_->setBrush(brush);
}

void
CanvasWidget::
setPen(const QPen &pen)
{
  assert(painter_);

  painter_->setPen(pen);
}

void
CanvasWidget::
drawPath(const QString &path)
{
  assert(painter_);

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

  Visitor visitor(painter_, displayRange_);

  CSVGUtil::visitPath(path.toStdString(), visitor);
}

//---

WebWidget::
WebWidget(Area *area, const QString &addr) :
 Widget(area), addr_(addr)
{
  setObjectName("web");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

#ifdef USE_WEBENGINE
  web_ = new QWebEngineView;
#else
  web_ = new QWebView;
#endif

  web_->load(QUrl(addr_));

  layout->addWidget(web_);
}

QString
WebWidget::
id() const
{
  return QString("web.%1").arg(pos());
}

void
WebWidget::
draw(QPainter *)
{
}

QSize
WebWidget::
calcSize() const
{
  return QSize(-1, 400);
}

//---

MarkdownWidget::
MarkdownWidget(Area *area, const FileText &fileText) :
 Widget(area)
{
  setObjectName("markdown");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  textEdit_ = new QTextEdit;

  textEdit_->setObjectName("edit");
  textEdit_->setReadOnly(true);

  layout->addWidget(textEdit_);

  setFileText(fileText);
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

    if (! fileToLines(fileText.value, lines))
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
draw(QPainter *)
{
}

QSize
MarkdownWidget::
calcSize() const
{
  auto *doc    = textEdit_->document();
  auto *layout = doc->documentLayout();

  int h = layout->documentSize().height() + 8;

  return QSize(-1, h);
}

//---

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

    if (! fileToLines(fileText.value, lines))
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

//---

SVGWidget::
SVGWidget(Area *area, const FileText &fileText) :
 Widget(area), fileText_(fileText)
{
  setObjectName("svg");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);
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
  if      (name == "width")
    value = width_;
  else if (name == "height")
    value = height_;
  else
    return false;

  return true;
}

bool
SVGWidget::
setNameValue(const QString &name, const QVariant &value)
{
  bool ok { true };

  if      (name == "width")
    width_ = value.toInt(&ok);
  else if (name == "height")
    height_ = value.toInt(&ok);
  else
    return false;

  if (! ok)
    return false;

  return true;
}

void
SVGWidget::
draw(QPainter *painter)
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
calcSize() const
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

  if (width_  > 0) width  = width_;
  if (height_ > 0) height = height_;

  return QSize(width, height);
}

//---

Status::
Status(Frame *frame) :
 QFrame(nullptr), frame_(frame)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void
Status::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  std::string dirname;

  (void) COSFile::getCurrentDir(dirname);

  QFontMetrics fm(font());

  painter.drawText(margin_, margin_ + fm.ascent(), dirname.c_str());
}

QSize
Status::
sizeHint() const
{
  const auto &margins = contentsMargins();

  int ym = margins.top() + margins.bottom() + 2*margin_;

  //---

  QFontMetrics fm(font());

  return QSize(-1, fm.height() + ym);
}

//---

void
TclImageCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file"  , TclCmdArg::Type::String , "file name");
  argv.addCmdArg("-width" , TclCmdArg::Type::Integer, "width");
  argv.addCmdArg("-height", TclCmdArg::Type::Integer, "height");
}

QStringList
TclImageCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclImageCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto fileName = argv.getParseStr("file");

  auto *area = frame()->lscroll()->area();

  auto *widget = area->addImageWidget(fileName);

  //---

  if (argv.hasParseArg("width")) {
    int width = argv.getParseInt("width");

    widget->setWidth(width);
  }

  if (argv.hasParseArg("height")) {
    int height = argv.getParseInt("height");

    widget->setHeight(height);
  }

  return frame()->setCmdRc(widget->id());
}

//---

void
TclCanvasCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-width" , TclCmdArg::Type::Integer, "width");
  argv.addCmdArg("-height", TclCmdArg::Type::Integer, "height");
  argv.addCmdArg("-proc"  , TclCmdArg::Type::String , "draw proc");
}

QStringList
TclCanvasCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclCanvasCmd::
exec(TclCmdArgs &argv)
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

  width  = std::max(width , 100);
  height = std::max(height, 100);

  auto *area = frame()->lscroll()->area();

  auto *canvasWidget = area->addCanvasWidget(width, height);

  if (argv.hasParseArg("proc"))
    canvasWidget->setDrawProc(argv.getParseStr("proc"));

  return frame()->setCmdRc(canvasWidget->id());

  return true;
}

//---

void
TclWebCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-addr", TclCmdArg::Type::String, "web address");
}

QStringList
TclWebCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclWebCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto addr = argv.getParseStr("addr");

  auto *area = frame()->lscroll()->area();

  auto *widget = area->addWebWidget(addr);

  return frame()->setCmdRc(widget->id());
}

//---

void
TclMarkdownCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "markdown file");
  argv.addCmdArg("-text", TclCmdArg::Type::String, "markdown text");
}

QStringList
TclMarkdownCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclMarkdownCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  MarkdownWidget *widget = nullptr;

  auto *area = frame()->lscroll()->area();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = area->addMarkdownWidget(FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = area->addMarkdownWidget(FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame()->setCmdRc(widget->id());
}

//---

void
TclHtmlCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "html file");
  argv.addCmdArg("-text", TclCmdArg::Type::String, "html text");
}

QStringList
TclHtmlCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclHtmlCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  HtmlWidget *widget = nullptr;

  auto *area = frame()->lscroll()->area();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = area->addHtmlWidget(FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = area->addHtmlWidget(FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame()->setCmdRc(widget->id());
}

//---

void
TclSVGCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "svg file");
  argv.addCmdArg("-text", TclCmdArg::Type::String, "svg text");
}

QStringList
TclSVGCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclSVGCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  SVGWidget *widget = nullptr;

  auto *area = frame()->lscroll()->area();

  if      (argv.hasParseArg("file")) {
    auto file = argv.getParseStr("file");

    widget = area->addSVGWidget(FileText(FileText::Type::FILENAME, file));
  }
  else if (argv.hasParseArg("text")) {
    auto text = argv.getParseStr("text");

    widget = area->addSVGWidget(FileText(FileText::Type::TEXT, text));
  }
  else
    return false;

  return frame()->setCmdRc(widget->id());
}

//---

void
TclModelCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "model file");

  argv.addCmdArg("-comment_header"   , TclCmdArg::Type::Boolean,
                 "first comment line is header");
  argv.addCmdArg("-first_line_header", TclCmdArg::Type::Boolean,
                 "first line is header");
}

QStringList
TclModelCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclModelCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto fileName = argv.getParseStr("file");

  auto *model = new CQCsvModel();

  if (argv.hasParseArg("comment_header"))
    model->setCommentHeader(true);

  if (argv.hasParseArg("first_line_header"))
    model->setFirstLineHeader(true);

  if (! model->load(fileName)) {
    delete model;
    return false;
  }

  auto id = frame()->addModel(model);

  return frame()->setCmdRc(id);
}

//---

void
TclGetModelDataCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-model" , TclCmdArg::Type::String, "model name");
  argv.addCmdArg("-name"  , TclCmdArg::Type::String, "data name");
  argv.addCmdArg("-column", TclCmdArg::Type::String, "column name");
}

QStringList
TclGetModelDataCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclGetModelDataCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto name = argv.getParseStr("name");

  if (name == "?") {
    auto strs = QStringList() << "num_columns" << "num_rows" << "num_unique";
    return frame()->setCmdRc(strs);
  }

  //---

  auto modelName = argv.getParseStr("model");

  auto *model = frame()->getModel(modelName);
  if (! model) return false;

  if      (name == "num_columns")
    return frame()->setCmdRc(model->columnCount());
  else if (name == "num_rows")
    return frame()->setCmdRc(model->rowCount());

  auto *details = frame()->getModelDetails(modelName);
  assert(details);

  int                   column        = -1;
  CQModelColumnDetails *columnDetails = nullptr;

  if (argv.hasParseArg("column")) {
    auto columnName = argv.getParseStr("column");

    column = details->lookupColumn(columnName);

    columnDetails = details->columnDetails(column);
  }

  if (name == "num_unique") {
    if (! columnDetails)
      return false;

    return frame()->setCmdRc(columnDetails->numUnique());
  }

  return true;
}

//---

void
TclGetDataCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-id"  , TclCmdArg::Type::String, "object id");
  argv.addCmdArg("-name", TclCmdArg::Type::String, "name");
}

QStringList
TclGetDataCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclGetDataCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto id = argv.getParseStr("id");

  auto *larea = frame()->lscroll()->area();
  auto *rarea = frame()->rscroll()->area();

  auto *widget = larea->getWidget(id);

  if (! widget)
    widget = rarea->getWidget(id);

  if (! widget)
    return false;

  //---

  auto name = argv.getParseStr("name");

  QVariant value;

  if (! widget->getNameValue(name, value))
    return false;

  return frame()->setCmdRc(value);
}

//---

void
TclSetDataCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-id"   , TclCmdArg::Type::String, "object id");
  argv.addCmdArg("-name" , TclCmdArg::Type::String, "name");
  argv.addCmdArg("-value", TclCmdArg::Type::String, "value");
}

QStringList
TclSetDataCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclSetDataCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto id = argv.getParseStr("id");

  auto *larea = frame()->lscroll()->area();
  auto *rarea = frame()->rscroll()->area();

  auto *widget = larea->getWidget(id);

  if (! widget)
    widget = rarea->getWidget(id);

  if (! widget)
    return false;

  //---

  auto name  = argv.getParseStr("name");
  auto value = argv.getParseStr("value");

  if (! widget->setNameValue(name, value))
    return false;

  return true;
}

//---

void
TclShowModelCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-model", TclCmdArg::Type::String, "model name");
}

QStringList
TclShowModelCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclShowModelCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto modelName = argv.getParseStr("model");

  auto *model = frame()->getModel(modelName);
  if (! model) return false;

  //---

  std::cout << "<html>\n";
  std::cout << "<table width='100%' border>\n";

  int nr = std::min(model->rowCount(), 10);
  int nc = model->columnCount();

  for (int r = -1; r < nr; ++r) {
    std::cout << "<tr>\n";

    for (int c = 0; c < nc; ++c) {
      QString str;
      bool    ok;

      if (r < 0) {
        str = CQModelUtil::modelHeaderString(model, c, ok);

        std::cout << "<th>" << str.toStdString() << "</th>\n";
      }
      else {
        str = CQModelUtil::modelString(model, model->index(r, c, QModelIndex()), ok);

        std::cout << "<td>" << str.toStdString() << "</td>\n";
      }
    }

    std::cout << "</tr>\n";
  }

  std::cout << "</table>\n";

  return true;
}

//---

void
TclCanvasInstCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-path"  , TclCmdArg::Type::String, "path");
  argv.addCmdArg("-fill"  , TclCmdArg::Type::String, "fill");
  argv.addCmdArg("-stroke", TclCmdArg::Type::String, "stroke");
}

QStringList
TclCanvasInstCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclCanvasInstCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto *canvas = frame()->canvas();
  if (! canvas) return false;

  //---

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

      if (! stringToBool(strs[1], &ok))
        brush.setStyle(Qt::NoBrush);
    }
    else if (strs[0] == "color") {
      brush.setColor(QColor(strs[1]));
    }
  }

  canvas->setBrush(brush);

  //---

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

      if (! stringToBool(strs[1], &ok))
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

  //---

  auto path = argv.getParseStr("path");

  canvas->drawPath(path);

  return true;
}

}
