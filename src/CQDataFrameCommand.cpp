#include <CQDataFrameCommand.h>
#include <CQDataFrameTcl.h>
#include <CQDataFrameUnix.h>
#include <CQDataFrame.h>
#include <CQDataFrameHtml.h>
#include <CQDataFrameSVG.h>

#include <CQStrUtil.h>
#include <CTclParse.h>
#include <CFile.h>
#include <COSFile.h>

#include <QPainter>
#include <QMouseEvent>

namespace CQDataFrame {

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
draw(QPainter *painter, int dx, int dy)
{
  int x = dx;
  int y = dy;

  //---

  // draw lines (above command line)
  int numLines = int(lines_.size());

  for (int i = 0; i < numLines; ++i) {
    auto *line = lines_[size_t(i)];

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
  int x = 0;

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
  int x = 0;

  //---

  int numLines = int(lines_.size());

  painter->setPen(bgColor_);

  for (int i = lineNum1; i <= lineNum2; ++i) {
    QString text;
    int     tx, ty;

    // continuation line
    if      (i >= 0 && i < numLines) {
      auto *line = lines_[size_t(i)];

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

  auto *frame = this->frame();

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

    for (int pos1 = commandPos + int(command.length()); pos1 < line.length(); ++pos1) {
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

        pos1 = token1->pos() + int(token1->str().length()); // move to end
      }
      else {
        if (lastOption != "")
          optionValues[lastOption] = str;

        pos1 = token1->pos() + int(token1->str().length()); // move to end
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
CommandWidget::
mouseMoveEvent(QMouseEvent *e)
{
  if (mouseData_.pressed) {
    (void) pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

    update();
  }
}

void
CommandWidget::
mouseReleaseEvent(QMouseEvent *e)
{
  if (mouseData_.pressed)
    (void) pixelToText(e->pos(), mouseData_.moveLineNum, mouseData_.moveCharNum);

  mouseData_.pressed = false;
}

bool
CommandWidget::
pixelToText(const QPoint &p, int &lineNum, int &charNum)
{
  if (Widget::pixelToText(p, lineNum, charNum))
    return true;

  int numLines = int(lines_.size());

  // current line
  int y1 = numLines*charData_.height;
  int y2 = y1 + charData_.height - 1;

  if (p.y() >= y1 && p.y() <= y2) {
    int x = 0;

    lineNum = numLines;
    charNum = (p.x() - x)/charData_.width;

    return true;
  }

  return false;
}

bool
CommandWidget::
event(QEvent *e)
{
  if (e->type() == QEvent::KeyPress) {
    auto *ke = static_cast<QKeyEvent *>(e);

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

      placeWidgets();
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
  auto tline = line.trimmed();

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

    processCdCommand(path.c_str());
  }
  // process unix command
  else if (name[0] == '!') {
    // run command
    auto cmd = name.substr(1);

    processUnixCommand(cmd.c_str(), args);
  }
  // process tcl expr
  else if (tline[0] == '%') {
    auto cmd = tline.mid(1);

    processTclCommand(cmd, /*expr*/true);
  }
  // process tcl command
  else {
    processTclCommand(line, /*expr*/false);
  }
}

void
CommandWidget::
processCdCommand(const QString &path)
{
  auto path1 = path;

  std::string path2;

  if (CFile::expandTilde(path.toStdString(), path2))
    path1 = path2.c_str();

  if (COSFile::changeDir(path1.toStdString()))
    this->frame()->status()->update();
  else {
    QString res = "Failed to change directory";

    auto *text = area()->addTextWidget(res);

    text->setIsError(true);
  }
}

void
CommandWidget::
processUnixCommand(const QString &cmd, const Args &args)
{
  QString res;

  bool rc = runUnixCommand(cmd.toStdString(), args, res);

  if      (rc && res.startsWith("<html>")) {
    (void) makeWidget<HtmlWidget>(area(), FileText(FileText::Type::TEXT, res));
  }
  else if (rc && res.startsWith("<svg>")) {
    (void) makeWidget<SVGWidget>(area(), FileText(FileText::Type::TEXT, res));
  }
  else {
    auto *widget = makeWidget<UnixWidget>(area(), cmd, args, res);

    widget->setIsError(! rc);
  }
}

void
CommandWidget::
processTclCommand(const QString &cmd, bool expr)
{
  auto cmd1 = (expr ? "expr {" + cmd + "}" : cmd);

  QString res;

  bool rc = runTclCommand(cmd1, res);

  if      (rc && res.startsWith("<html>")) {
    (void) makeWidget<HtmlWidget>(area(), FileText(FileText::Type::TEXT, res));
  }
  else if (rc && res.startsWith("<svg>")) {
    (void) makeWidget<SVGWidget>(area(), FileText(FileText::Type::TEXT, res));
  }
  else {
    auto *widget = makeWidget<TclWidget>(area(), cmd, expr, res);

    widget->setIsError(! rc);
  }
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

  int numLines = int(lines_.size());

  QString str;

  for (int i = lineNum1; i <= lineNum2; ++i) {
    QString text;

    // continuation line
    if      (i >= 0 && i < numLines) {
      auto *line = lines_[size_t(i)];

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

  int numLines = int(lines_.size());

  for (int i = 0; i < numLines; ++i) {
    auto *line = lines_[size_t(i)];

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
contentsSizeHint() const
{
  int h = charData_.height;

  auto numLines = lines_.size();

  numLines = std::min(numLines, 25UL);

  h += int(numLines)*charData_.height;

  return QSize(-1, h);
}

QSize
CommandWidget::
contentsSize() const
{
  int h = charData_.height;

  auto numLines = lines_.size();

  h += int(numLines)*charData_.height;

  return QSize(-1, h);
}

}
