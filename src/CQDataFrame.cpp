#include <CQDataFrame.h>

#include <CQTabSplit.h>
#include <CQStrUtil.h>

#include <CFileMatch.h>
#include <CFile.h>
#include <COSFile.h>

#include <QFileDialog>
#include <QVBoxLayout>
#include <QPainter>
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include <QWheelEvent>

namespace CQDataFrame {

//------

QStringList
Frame::
s_completeFile(const QString &file)
{
  QStringList strs;

  CFileMatch fileMatch;

  std::vector<std::string> files;

  if (! fileMatch.matchPrefix(file.toStdString(), files))
    return strs;

  for (const auto &file : files)
    strs << file.c_str();

  return strs;
}

bool
Frame::
s_fileToLines(const QString &fileName, QStringList &lines)
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

bool
Frame::
s_stringToBool(const QString &str, bool *ok)
{
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

//------

Frame::
Frame(QWidget *parent) :
 QFrame(parent)
{
  setObjectName("dataFrame");

  auto *layout = new QVBoxLayout(this);
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

  mgr_ = new CQTclCmd::Mgr(qtcl_);

  mgr_->addCommand("help", new HelpTclCmd(this));

  mgr_->addCommand("complete", new CompleteTclCmd(this));

  mgr_->addCommand("get_data", new GetDataTclCmd(this));
  mgr_->addCommand("set_data", new SetDataTclCmd(this));
}

Frame::
~Frame()
{
  delete mgr_;
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
init()
{
}

void
Frame::
addWidgetFactory(WidgetFactory *factory)
{
  widgetFactories_[factory->name()] = factory;

  factory->addTclCommand(this);
}

WidgetFactory *
Frame::
getWidgetFactory(const QString &name) const
{
  for (const auto &pf : widgetFactories_)
    if (pf.first == name)
      return pf.second;

  return nullptr;
}

//---

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

  if (! s_fileToLines(fileName, lines))
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
  if (scroll_->isCommand()) {
    command_ = new CommandWidget(this);

    addWidget(command_);
  }
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

TextWidget *
Area::
addTextWidget(const QString &text)
{
  auto *widget = new TextWidget(this, text);

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

//---

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

//------

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
HelpTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-hidden" , ArgType::Boolean, "show hidden");
  addArg(argv, "-verbose", ArgType::Boolean, "verbose help");
}

QStringList
HelpTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
HelpTclCmd::
exec(CQTclCmd::CmdArgs &argv)
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
    mgr_->help(pattern, verbose, hidden);
  else
    mgr_->helpAll(verbose, hidden);

  return true;
}

//---

void
CompleteTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-command", ArgType::String, "complete command").setRequired();

  addArg(argv, "-option"     , ArgType::String , "complete option");
  addArg(argv, "-value"      , ArgType::String , "complete value");
  addArg(argv, "-name_values", ArgType::String , "option name values");
  addArg(argv, "-all"        , ArgType::Boolean, "get all matches");
  addArg(argv, "-exact_space", ArgType::Boolean, "add space if exact");
}

QStringList
CompleteTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
CompleteTclCmd::
exec(CQTclCmd::CmdArgs &argv)
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
    auto *proc = frame_->tclCmdMgr()->getCommand(command);
    if (! proc) return false;

    //---

    // get arg info
    Vars vars;

    CQTclCmd::CmdArgs args(command, vars);

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

      if      (type == int(ArgType::Boolean)) {
      }
      else if (type == int(ArgType::Integer)) {
      }
      else if (type == int(ArgType::Real)) {
      }
      else if (type == int(ArgType::SBool)) {
        strs << "0" << "1";
      }
      else if (type == int(ArgType::String)) {
        strs = proc->getArgValues(option, nameValueMap);
      }
      else {
        return frame_->setCmdRc(strs);
      }

      //---

      if (allFlag)
        return frame_->setCmdRc(strs);

      auto matchValues = CQStrUtil::matchStrs(value, strs);

      bool exact;

      auto newValue = CQStrUtil::longestMatch(matchValues, exact);

      if (newValue.length() >= value.length()) {
        if (exact && exactSpace)
          newValue += " ";

        return frame_->setCmdRc(newValue);
      }

      return frame_->setCmdRc(strs);
    }
    else {
      const auto &names = args.getCmdArgNames();

      if (allFlag) {
        QStringList strs;

        for (const auto &name : names)
          strs.push_back(name.mid(1));

        return frame_->setCmdRc(strs);
      }

      auto matchArgs = CQStrUtil::matchStrs(optionStr, names);

      bool exact;

      auto newOption = CQStrUtil::longestMatch(matchArgs, exact);

      if (newOption.length() >= optionStr.length()) {
        if (exact && exactSpace)
          newOption += " ";

        return frame_->setCmdRc(newOption);
      }
    }
  }
  else {
    const auto &cmds = frame_->qtcl()->commandNames();

    auto matchCmds = CQStrUtil::matchStrs(command, cmds);

    if (allFlag)
      return frame_->setCmdRc(matchCmds);

    bool exact;

    auto newCommand = CQStrUtil::longestMatch(matchCmds, exact);

    if (newCommand.length() >= command.length()) {
      if (exact && exactSpace)
        newCommand += " ";

      return frame_->setCmdRc(newCommand);
    }
  }

  return frame_->setCmdRc(QString());
}

//------

void
GetDataTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-id"  , ArgType::String, "object id");
  addArg(argv, "-name", ArgType::String, "name");
}

QStringList
GetDataTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
GetDataTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto id = argv.getParseStr("id");

  auto *larea = frame_->larea();
  auto *rarea = frame_->rarea();

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

  return frame_->setCmdRc(value);
}

//---

void
SetDataTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-id"   , ArgType::String, "object id");
  addArg(argv, "-name" , ArgType::String, "name");
  addArg(argv, "-value", ArgType::String, "value");
}

QStringList
SetDataTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
SetDataTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto id = argv.getParseStr("id");

  auto *larea = frame_->larea();
  auto *rarea = frame_->rarea();

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

}
