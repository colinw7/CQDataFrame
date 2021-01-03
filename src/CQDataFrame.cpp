#include <CQDataFrame.h>
#include <CQTclUtil.h>
#include <CQTabSplit.h>
#include <CQUtil.h>
#include <CCommand.h>

#ifdef MARKDOWN_DATA
#include <CMarkdown.h>
#endif

#ifdef MODEL_DATA
#include <CQCsvModel.h>
#include <CQModelDetails.h>
#include <CQModelUtil.h>
#endif

#include <CQStrParse.h>
#include <CQStrUtil.h>

#ifdef FILE_MGR_DATA
#include <CQFileBrowser.h>
#include <CFileBrowser.h>
#include <CQDirView.h>
#include <CFileUtil.h>
#endif

#include <CTclParse.h>
#include <CSVGUtil.h>
#include <CFileMatch.h>
#include <CFile.h>

#ifdef FILE_DATA
//#include <CQEdit.h>
#include <CQVi.h>
#endif

#ifdef ESCAPE_PARSER
#include <CEscapeParse.h>
#endif

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

#ifdef ESCAPE_PARSER
class EscapeParse : public CEscapeParse {
 public:
  EscapeParse() { }

 ~EscapeParse() { }

  const std::string &str() const { return str_; }

  void handleChar(char c) {
    str_ +=  c;
  }

  void handleGraphic(char) {
  }

  void handleEscape(const CEscapeData *e) {
    if      (e->type == CEscapeType::BS ) { }
    else if (e->type == CEscapeType::HT ) { str_ += '\t'; }
    else if (e->type == CEscapeType::LF ) { str_ += '\n'; }
    else if (e->type == CEscapeType::VT ) { str_ += '\v'; }
    else if (e->type == CEscapeType::FF ) { str_ += '\f'; }
    else if (e->type == CEscapeType::CR ) { str_ += '\r'; }
    else if (e->type == CEscapeType::SGR) {
    }
  }

  void log(const std::string &) const { }

  void logError(const std::string &) const { }

  std::string waitMessage(const char *, uint) { return ""; }

 private:
  std::string str_;
};
#endif

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

QStringList completeFile(const QString &file) {
  QStringList strs;

  CFileMatch fileMatch;

  std::vector<std::string> files;

  if (! fileMatch.matchPrefix(file.toStdString(), files))
    return strs;

  for (const auto &file : files)
    strs << file.c_str();

  return strs;
}

//---

class TclCmd {
 public:
  using Vars = std::vector<QVariant>;

 public:
  TclCmd(Frame *frame, const QString &name) :
   frame_(frame), name_(name) {
    auto *qtcl = frame_->qtcl();

    cmdId_ = qtcl->createObjCommand(name_,
      (CQTcl::ObjCmdProc) &TclCmd::commandProc, (CQTcl::ObjCmdData) this);
  }

  static int commandProc(ClientData clientData, Tcl_Interp *, int objc, const Tcl_Obj **objv) {
    auto *command = (TclCmd *) clientData;

    auto *qtcl = command->frame_->qtcl();

    Vars vars;

    for (int i = 1; i < objc; ++i) {
      auto *obj = const_cast<Tcl_Obj *>(objv[i]);

      vars.push_back(qtcl->variantFromObj(obj));
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

void
Frame::
init()
{
  static bool initialized;

  if (! initialized) {
#ifdef FILE_DATA
    //CQEdit::init();
#endif

#ifdef FILE_MGR_DATA
    CQDirViewFactory::init();
#endif

    initialized = true;
  }
}

Frame::
Frame(QWidget *parent) :
 QFrame(parent)
{
  Frame::init();

  //---

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

  addTclCommand("help"    , new TclHelpCmd    (this));
  addTclCommand("complete", new TclCompleteCmd(this));

  addTclCommand("image" , new TclImageCmd (this));
  addTclCommand("canvas", new TclCanvasCmd(this));
  addTclCommand("web"   , new TclWebCmd   (this));
  addTclCommand("html"  , new TclHtmlCmd  (this));
  addTclCommand("svg"   , new TclSVGCmd   (this));

#ifdef FILE_DATA
  addTclCommand("file", new TclFileCmd(this));
#endif

#ifdef MARKDOWN_DATA
  addTclCommand("markdown", new TclMarkdownCmd(this));
#endif

#ifdef FILE_MGR_DATA
  addTclCommand("filemgr", new TclFileMgrCmd (this));
#endif

  addTclCommand("get_data", new TclGetDataCmd(this));
  addTclCommand("set_data", new TclSetDataCmd(this));

#ifdef MODEL_DATA
  addTclCommand("model", new TclModelCmd(this));

  addTclCommand("get_model_data", new TclGetModelDataCmd(this));
  addTclCommand("show_model"    , new TclShowModelCmd   (this));
#endif
}

Frame::
~Frame()
{
  delete qtcl_;
}

//---

Area *
Frame::
larea() const
{
  return lscroll_->area();
}

Area *
Frame::
rarea() const
{
  return rscroll_->area();
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

TclCmdProc *
Frame::
getTclCommand(const QString &name) const
{
  auto p = commandProcs_.find(name);

  return (p != commandProcs_.end() ? (*p).second : nullptr);
}

//------

bool
Frame::
help(const QString &pattern, bool verbose, bool hidden)
{
  using Procs = std::vector<TclCmdProc *>;

  Procs procs;

  auto p = commandProcs_.find(pattern);

  if (p != commandProcs_.end()) {
    procs.push_back((*p).second);
  }
  else {
    QRegExp re(pattern, Qt::CaseSensitive, QRegExp::Wildcard);

    for (auto &p : commandProcs_) {
      if (re.exactMatch(p.first))
        procs.push_back(p.second);
    }
  }

  if (procs.empty()) {
    std::cout << "Command not found\n";
    return false;
  }

  for (const auto &proc : procs) {
    if (verbose) {
      Vars vars;

      TclCmdArgs args(proc->name(), vars);

      proc->addArgs(args);

      args.help(hidden);
    }
    else {
      std::cout << proc->name().toStdString() << "\n";
    }
  }

  return true;
}

void
Frame::
helpAll(bool verbose, bool hidden)
{
  // all procs
  for (auto &p : commandProcs_) {
    auto *proc = p.second;

    if (verbose) {
      Vars vars;

      TclCmdArgs args(proc->name(), vars);

      proc->addArgs(args);

      args.help(hidden);
    }
    else {
      std::cout << proc->name().toStdString() << "\n";
    }
  }
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

  larea()->save(os);
  rarea()->save(os);
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

  auto *area = larea();

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

  area->scrollToEnd();

  return true;
}

#ifdef MODEL_DATA
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
#endif

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

  return scroll()->frame()->larea()->commandWidget();
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
addTclWidget(const QString &cmd, const QString &res)
{
  auto *widget = new TclWidget(this, cmd, res);

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

#ifdef MARKDOWN_DATA
MarkdownWidget *
Area::
addMarkdownWidget(const FileText &fileText)
{
  auto *widget = new MarkdownWidget(this, fileText);

  addWidget(widget);

  return widget;
}
#endif

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

#ifdef FILE_DATA
FileWidget *
Area::
addFileWidget(const QString &filename)
{
  auto *widget = new FileWidget(this, filename);

  addWidget(widget);

  return widget;
}
#endif

#ifdef FILE_MGR_DATA
FileMgrWidget *
Area::
addFileMgrWidget()
{
  auto *widget = new FileMgrWidget(this);

  addWidget(widget);

  return widget;
}
#endif

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
scrollToEnd()
{
  scroll()->ensureVisible(0, scroll()->getYSize());
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
      int h1 = size.height(); if (h1 <= 0) h1 = widget->defaultWidth();

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
      int w1 = size.width (); if (w1 <= 0) w1 = widget->defaultWidth ();
      int h1 = size.height(); if (h1 <= 0) h1 = widget->defaultHeight();

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
  auto *frame = area()->scroll()->frame();

  return frame->larea();
}

Area *
Widget::
rarea() const
{
  auto *frame = area()->scroll()->frame();

  return frame->rarea();
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
    return QSize(-1, 20);

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
TclWidget(Area *area, const QString &cmd, const QString &res) :
 TextWidget(area, res), cmd_(cmd)
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

  if (command->runTclCommand(cmd_, res))
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
  int len = line.length();

  if (pos >= len)
    pos = len - 1;

  //---

  CTclParse parse;

  CTclParse::Tokens tokens;

  parse.parseLine(line.toStdString(), tokens);

  auto *token = parse.getTokenForPos(tokens, pos);

  //---

  auto *frame = area()->scroll()->frame();

  auto *qtcl = frame->qtcl();

  //---

  auto lhs = line.mid(0, token ? token->pos() : pos + 1);
  auto str = (token ? token->str() : "");
  auto rhs = line.mid(token ? token->endPos() + 1 : pos + 1);

  // complete command
  if      (token && token->type() == CTclToken::Type::COMMAND) {
  //std::cerr << "Command: " << str << "\n";

    const auto &cmds = qtcl->commandNames();

    auto matchCmds = CQStrUtil::matchStrs(str.c_str(), cmds);

    QString matchStr;
    bool    exact = false;

#if 0
    if (interactive && matchCmds.size() > 1) {
      matchStr = showCompletionChooser(matchCmds);

      if (matchStr != "")
        exact = true;
    }
#endif

    //---

    newText = lhs;

    if (matchStr == "")
      newText += CQStrUtil::longestMatch(matchCmds, exact);
    else
      newText += matchStr;

    if (exact)
      newText += " ";

    newText += rhs;

    return (newText.length() > line.length());
  }
  // complete option
  else if (str[0] == '-') {
    // get previous command token for command name
    std::string command;

    for (int pos1 = pos - 1; pos1 >= 0; --pos1) {
      auto *token1 = parse.getTokenForPos(tokens, pos1);
      if (! token1) continue;

      if (token1->type() == CTclToken::Type::COMMAND) {
        command = token1->str();
        break;
      }
    }

    if (command == "")
      return false;

    auto option = str.substr(1);

    //---

    // get all options for interactive complete
    QString matchStr;

#if 0
    if (interactive) {
      auto cmd = QString("complete -command {%1} -option {*} -all").
                         arg(command.c_str());

      QVariant res;

      (void) qtcl->eval(cmd, res);

      QStringList strs = resultToStrings(res);

      auto matchStrs = CQStrUtil::matchStrs(option.c_str(), strs);

      if (matchStrs.size() > 1) {
        matchStr = showCompletionChooser(matchStrs);

        if (matchStr != "")
          matchStr = "-" + matchStr + " ";
      }
    }
#endif

    //---

    if (matchStr == "") {
      // use complete command to complete command option
      auto cmd = QString("complete -command {%1} -option {%2} -exact_space").
                         arg(command.c_str()).arg(option.c_str());

      QVariant res;

      (void) qtcl->eval(cmd, res);

      matchStr = res.toString();
    }

    newText = lhs + matchStr + rhs;

    return (newText.length() > line.length());
  }
  else {
    // get previous tokens for option name and command name
    using OptionValues = std::map<std::string, std::string>;

    std::string  command;
    int          commandPos { -1 };
    std::string  option;
    OptionValues optionValues;

    for (int pos1 = pos - 1; pos1 >= 0; --pos1) {
      auto *token1 = parse.getTokenForPos(tokens, pos1);
      if (! token1) continue;

      const auto &str = token1->str();
      if (str.empty()) continue;

      if      (token1->type() == CTclToken::Type::COMMAND) {
        command    = str;
        commandPos = token1->pos();
        break;
      }
      else if (str[0] == '-') {
        if (option.empty())
          option = str.substr(1);

        if (pos1 > token1->pos())
          pos1 = token1->pos(); // move to start
      }
    }

    if (command == "" || option == "")
      return false;

    // get option values to next command
    std::string lastOption;

    for (int pos1 = commandPos + command.length(); pos1 < line.length(); ++pos1) {
      auto *token1 = parse.getTokenForPos(tokens, pos1);
      if (! token1) continue;

      const auto &str = token1->str();
      if (str.empty()) continue;

      if      (token1->type() == CTclToken::Type::COMMAND) {
        break;
      }
      else if (str[0] == '-') {
        lastOption = str.substr(1);

        optionValues[lastOption] = "";

        pos1 = token1->pos() + token1->str().length(); // move to end
      }
      else {
        if (lastOption != "")
          optionValues[lastOption] = str;

        pos1 = token1->pos() + token1->str().length(); // move to end
      }
    }

    //---

    std::string nameValues;

    for (const auto &nv : optionValues) {
      if (! nameValues.empty())
        nameValues += " ";

      nameValues += "{{" + nv.first + "} {" + nv.second + "}}";
    }

    nameValues = "{" + nameValues + "}";

    //---

    // get all option values for interactive complete
    QString matchStr;

#if 0
    if (interactive) {
      auto cmd =
        QString("complete -command {%1} -option {%2} -value {*} -name_values %3 -all").
                arg(command.c_str()).arg(option.c_str()).arg(nameValues.c_str());

      QVariant res;

      (void) qtcl->eval(cmd, res);

      QStringList strs = resultToStrings(res);

      auto matchStrs = CQStrUtil::matchStrs(str.c_str(), strs);

      if (matchStrs.size() > 1) {
        matchStr = showCompletionChooser(matchStrs);

        if (matchStr != "")
          matchStr = matchStr + " ";
      }
    }
#endif

    //---

    if (matchStr == "") {
      // use complete command to complete command option value
      QVariant res;

      auto cmd =
        QString("complete -command {%1} -option {%2} -value {%3} -name_values %4 -exact_space").
                arg(command.c_str()).arg(option.c_str()).arg(str.c_str()).arg(nameValues.c_str());

      qtcl->eval(cmd, res);

      matchStr = res.toString();
    }

    newText = lhs + matchStr + rhs;

    return (newText.length() > line.length());
  }

  return false;
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

        area()->scrollToEnd();

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

  //---

#ifdef ESCAPE_PARSER
  EscapeParse eparse;

  for (std::size_t i = 0; i < res.size(); ++i)
    eparse.addOutputChar(res[i]);

  res = eparse.str();
#endif

  return true;
}

bool
CommandWidget::
runTclCommand(const QString &line, QString &res) const
{
  auto *frame = area()->scroll()->frame();

  auto *qtcl = frame->qtcl();

  COSExec::grabOutput();

  bool log = true;

  int tclRc = qtcl->eval(line, /*showError*/true, /*showResult*/log);

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

//---

CanvasWidget::
CanvasWidget(Area *area, int width, int height) :
 Widget(area)
{
  setObjectName("canvas");

  setWidth (width );
  setHeight(height);

  setWindowRange();
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
handleResize(int w, int h)
{
  const auto &margins = contentsMargins();

  int l = margins.left(), r = margins.right ();
  int t = margins.top (), b = margins.bottom();

  int iw = w - l - r;
  int ih = h - t - b;

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
draw(QPainter *painter)
{
  if (dirty_) {
    ipainter_ = new QPainter;

    ipainter_->begin(&image_);

    ipainter_->fillRect(QRect(0, 0, image_.width(), image_.height()), bgColor_);

    auto *frame = area()->scroll()->frame();

    frame->setCanvas(this);

    ipainter_->setPen  (Qt::red);
    ipainter_->setBrush(Qt::green);

    if (drawProc_ != "") {
      auto *qtcl = frame->qtcl();

      QString cmd = QString("%1 %2").arg(drawProc_).arg(id());

      bool log = true;

      (void) qtcl->eval(cmd, /*showError*/true, /*showResult*/log);
    }

    frame->setCanvas(nullptr);

    ipainter_->end();

    delete ipainter_;

    dirty_ = false;
  }

  const auto &margins = contentsMargins();

  int l = margins.left();
  int t = margins.top ();

  painter->drawImage(l, t, image_);
}

QSize
CanvasWidget::
calcSize() const
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

  ipainter_->drawPoint(px, py);
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

#ifdef MARKDOWN_DATA
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
#endif

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

  if (this->width () > 0) width  = this->width ();
  if (this->height() > 0) height = this->height();

  return QSize(width, height);
}

//---

#ifdef FILE_DATA
FileWidget::
FileWidget(Area *area, const QString &fileName) :
 Widget(area), fileName_(fileName)
{
  setObjectName("file");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  //edit_ = new CQEdit;

  //edit_->getFile()->loadLines(fileName.toStdString());

  edit_ = new CQVi;

  edit_->loadFile(fileName.toStdString());

  layout->addWidget(edit_);
}

QString
FileWidget::
id() const
{
  return QString("file.%1").arg(pos());
}

bool
FileWidget::
getNameValue(const QString &name, QVariant &value) const
{
  return Widget::getNameValue(name, value);
}

bool
FileWidget::
setNameValue(const QString &name, const QVariant &value)
{
  return Widget::setNameValue(name, value);
}

void
FileWidget::
draw(QPainter *)
{
}

QSize
FileWidget::
calcSize() const
{
  return QSize(-1, 400);
}
#endif

//---

#ifdef FILE_MGR_DATA
FileMgrWidget::
FileMgrWidget(Area *area) :
 Widget(area)
{
  setObjectName("filemgr");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  fileMgr_ = new CQFileBrowser;

  fileMgr_->getFileBrowser()->setShowDotDot(false);
  fileMgr_->getFileBrowser()->setFontSize(16);

  connect(fileMgr_, SIGNAL(fileActivated(const QString &)),
          this, SLOT(fileActivated(const QString &)));

  layout->addWidget(fileMgr_);
}

QString
FileMgrWidget::
id() const
{
  return QString("filemgr.%1").arg(pos());
}

bool
FileMgrWidget::
getNameValue(const QString &name, QVariant &value) const
{
  return Widget::getNameValue(name, value);
}

bool
FileMgrWidget::
setNameValue(const QString &name, const QVariant &value)
{
  return Widget::setNameValue(name, value);
}

void
FileMgrWidget::
draw(QPainter *)
{
}

QSize
FileMgrWidget::
calcSize() const
{
  return QSize(-1, 400);
}

void
FileMgrWidget::
fileActivated(const QString &filename)
{
  auto name = filename.toStdString();

  CFile file(name);

  CFileType type = CFileUtil::getType(&file);

  if (type & CFILE_TYPE_IMAGE) {
    (void) larea()->addImageWidget(filename);
  }
}

#endif

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
TclHelpCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-hidden" , TclCmdArg::Type::Boolean, "show hidden");
  argv.addCmdArg("-verbose", TclCmdArg::Type::Boolean, "verbose help");
}

QStringList
TclHelpCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclHelpCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  if (! argv.parse())
    return false;

  auto hidden  = argv.getParseBool("hidden");
  auto verbose = argv.getParseBool("verbose");

  //---

  const auto &pargs = argv.getParseArgs();

  QString pattern = (! pargs.empty() ? pargs[0].toString() : "");

  //---

  if (pattern.length())
    frame()->help(pattern, verbose, hidden);
  else
    frame()->helpAll(verbose, hidden);

  return true;
}

//---

void
TclCompleteCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-command", TclCmdArg::Type::String, "complete command").setRequired();

  argv.addCmdArg("-option"     , TclCmdArg::Type::String , "complete option");
  argv.addCmdArg("-value"      , TclCmdArg::Type::String , "complete value");
  argv.addCmdArg("-name_values", TclCmdArg::Type::String , "option name values");
  argv.addCmdArg("-all"        , TclCmdArg::Type::Boolean, "get all matches");
  argv.addCmdArg("-exact_space", TclCmdArg::Type::Boolean, "add space if exact");
}

QStringList
TclCompleteCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclCompleteCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  if (! argv.parse())
    return false;

  auto command    = argv.getParseStr ("command");
  auto allFlag    = argv.getParseBool("all");
  auto exactSpace = argv.getParseBool("exact_space");

  //---

  using NameValueMap = std::map<QString, QString>;

  NameValueMap nameValueMap;

  auto nameValues = argv.getParseStr("name_values");

  QStringList nameValues1;

  (void) CQTcl::splitList(nameValues, nameValues1);

  for (const auto &nv : nameValues1) {
    QStringList nameValues2;

    (void) CQTcl::splitList(nv, nameValues2);

    if (nameValues2.length() == 2)
      nameValueMap[nameValues2[0]] = nameValues2[1];
  }

  //---

  if (argv.hasParseArg("option")) {
    // get option to complete
    auto option = argv.getParseStr("option");

    auto optionStr = "-" + option;

    //---

    // get proc
    auto *proc = frame()->getTclCommand(command);
    if (! proc) return false;

    //---

    // get arg info
    Vars vars;

    TclCmdArgs args(command, vars);

    proc->addArgs(args);

    //---

    if (argv.hasParseArg("value")) {
      // get option arg type
      auto *arg = args.getCmdOpt(option);
      if (! arg) return false;

      auto type = arg->type();

      //---

      // get value to complete
      auto value = argv.getParseStr("value");

      //---

      // complete by type
      QStringList strs;

      if      (type == TclCmdArg::Type::Boolean) {
      }
      else if (type == TclCmdArg::Type::Integer) {
      }
      else if (type == TclCmdArg::Type::Real) {
      }
      else if (type == TclCmdArg::Type::SBool) {
        strs << "0" << "1";
      }
      else if (type == TclCmdArg::Type::String) {
        strs = proc->getArgValues(option, nameValueMap);
      }
      else {
        return frame()->setCmdRc(strs);
      }

      //---

      if (allFlag)
        return frame()->setCmdRc(strs);

      auto matchValues = CQStrUtil::matchStrs(value, strs);

      bool exact;

      auto newValue = CQStrUtil::longestMatch(matchValues, exact);

      if (newValue.length() >= value.length()) {
        if (exact && exactSpace)
          newValue += " ";

        return frame()->setCmdRc(newValue);
      }

      return frame()->setCmdRc(strs);
    }
    else {
      const auto &names = args.getCmdArgNames();

      if (allFlag) {
        QStringList strs;

        for (const auto &name : names)
          strs.push_back(name.mid(1));

        return frame()->setCmdRc(strs);
      }

      auto matchArgs = CQStrUtil::matchStrs(optionStr, names);

      bool exact;

      auto newOption = CQStrUtil::longestMatch(matchArgs, exact);

      if (newOption.length() >= optionStr.length()) {
        if (exact && exactSpace)
          newOption += " ";

        return frame()->setCmdRc(newOption);
      }
    }
  }
  else {
    const auto &cmds = frame()->qtcl()->commandNames();

    auto matchCmds = CQStrUtil::matchStrs(command, cmds);

    if (allFlag)
      return frame()->setCmdRc(matchCmds);

    bool exact;

    auto newCommand = CQStrUtil::longestMatch(matchCmds, exact);

    if (newCommand.length() >= command.length()) {
      if (exact && exactSpace)
        newCommand += " ";

      return frame()->setCmdRc(newCommand);
    }
  }

  return frame()->setCmdRc(QString());
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
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
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

  auto *area = frame()->larea();

  if (! argv.hasParseArg("file"))
    return false;

  auto fileName = argv.getParseStr("file");

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

  int minWidth  = 100;
  int minHeight = 100;

  width  = std::max(width , minWidth );
  height = std::max(height, minHeight);

  auto *area = frame()->larea();

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

  auto *area = frame()->larea();

  auto *widget = area->addWebWidget(addr);

  return frame()->setCmdRc(widget->id());
}

//---

#ifdef MARKDOWN_DATA
void
TclMarkdownCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "markdown file");
  argv.addCmdArg("-text", TclCmdArg::Type::String, "markdown text");
}

QStringList
TclMarkdownCmd::
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
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

  auto *area = frame()->larea();

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
#endif

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
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
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

  auto *area = frame()->larea();

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
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
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

  auto *area = frame()->larea();

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

#ifdef FILE_DATA
void
TclFileCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-file", TclCmdArg::Type::String, "svg file");
}

QStringList
TclFileCmd::
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
}

bool
TclFileCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  FileWidget *widget = nullptr;

  auto *area = frame()->larea();

  auto fileName = argv.getParseStr("file");

  widget = area->addFileWidget(fileName);

  return frame()->setCmdRc(widget->id());
}
#endif

//---

#ifdef FILE_MGR_DATA
void
TclFileMgrCmd::
addArgs(TclCmdArgs &)
{
}

QStringList
TclFileMgrCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
TclFileMgrCmd::
exec(TclCmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto *area = frame()->larea();

  auto *widget = area->addFileMgrWidget();

  return frame()->setCmdRc(widget->id());
}
#endif

//---

#ifdef MODEL_DATA
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
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return completeFile(file);
  }

  return strs;
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
#endif

//---

#ifdef MODEL_DATA
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
#endif

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

  auto *larea = frame()->larea();
  auto *rarea = frame()->rarea();

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

  auto *larea = frame()->larea();
  auto *rarea = frame()->rarea();

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

#ifdef MODEL_DATA
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
#endif

//---

void
TclCanvasInstCmd::
addArgs(TclCmdArgs &argv)
{
  argv.addCmdArg("-path"   , TclCmdArg::Type::String, "path");
  argv.addCmdArg("-pixel"  , TclCmdArg::Type::String, "pixel");

  argv.addCmdArg("-fill"   , TclCmdArg::Type::String, "fill");
  argv.addCmdArg("-stroke" , TclCmdArg::Type::String, "stroke");

  argv.addCmdArg("-mapping", TclCmdArg::Type::SBool , "mapping enabled");

  argv.addCmdArg("-pixel_to_window", TclCmdArg::Type::String , "pixel to window");
  argv.addCmdArg("-window_to_pixel", TclCmdArg::Type::String , "window to pixel");
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

        if (! stringToBool(strs[1], &ok))
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

    if (! stringToBool(str, &ok))
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

    frame()->setCmdRc(vars);
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

    frame()->setCmdRc(vars);
  }

  //---

  canvas->setMapping(true);

  return true;
}

}
