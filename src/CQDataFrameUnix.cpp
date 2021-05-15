#include <CQDataFrameUnix.h>
#include <CQDataFrame.h>
#include <CQIconButton.h>

#include <QTextEdit>
//#include <QAbstractTextDocumentLayout>
#include <QMenu>
#include <QPainter>

#include <svg/run_svg.h>

namespace CQDataFrame {

UnixWidget::
UnixWidget(Area *area, const QString &cmd, const Args &args, const QString &res) :
 TextWidget(area, res), cmd_(cmd), args_(args)
{
  setObjectName("unix");

  errMsg_ = "Error: command failed";
}

void
UnixWidget::
addWidgets()
{
  edit_ = new QTextEdit(this);

  edit_->setObjectName("edit");
  edit_->setText(cmdStr());
  edit_->setVisible(false);

  runButton_ = new CQIconButton(this);
  runButton_->setIcon("RUN");
  runButton_->setAutoRaise(true);
  runButton_->setToolTip("Run Command");
  runButton_->setVisible(false);

  connect(edit_, SIGNAL(textChanged()), this, SLOT(textChangedSlot()));

  connect(runButton_, SIGNAL(clicked()), this, SLOT(runCmdSlot()));
}

void
UnixWidget::
setEditing(bool b)
{
  TextWidget::setEditing(b);

  updateLayout();

  placeWidgets();
}

void
UnixWidget::
updateLayout()
{
  edit_     ->setVisible(isEditing());
  runButton_->setVisible(isEditing());

  if (isEditing()) {
    if (contentsMargins().top())
      setContentsMargins(0, 0, 0, 0);

    auto bs = runButton_->sizeHint();

    edit_->move  (margin_, height() - margin_ - bs.height());
    edit_->resize(width() - 2*margin_ - bs.width(), bs.height());

    runButton_->move  (edit_->x() + edit_->width(), edit_->y());
    runButton_->resize(bs.width(), bs.height());

    edit_     ->raise();
    runButton_->raise();

    setContentsMargins(0, 0, 0, bs.height() + margin_);
  }
  else {
    if (contentsMargins().top())
      setContentsMargins(0, 0, 0, 0);
  }
}

QString
UnixWidget::
cmdStr() const
{
  QString s = cmd_;

  for (auto &arg : args_)
    s += " " + QString(arg.c_str());

  return s;
}

void
UnixWidget::
setCmd(const QString &s)
{
  // parse command
  std::string              name;
  std::vector<std::string> args;

  parseCommand(s, name, args);

  cmd_  = name.c_str();
  args_ = args;

  QString res;

  bool rc = runUnixCommand(cmd_.toStdString(), args_, res);

  setText(res);

  setIsError(! rc);

  if (cmd_ != edit_->toPlainText())
    edit_->setText(cmdStr());

  emit contentsChanged();
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
textChangedSlot()
{
  QString s = edit_->toPlainText();

  if (s.length()) {
    // parse command
    std::string              name;
    std::vector<std::string> args;

    parseCommand(s, name, args);

    cmd_  = name.c_str();
    args_ = args;
  }
  else {
    cmd_  = "";
    args_.clear();
  }

  update();
}

void
UnixWidget::
runCmdSlot()
{
  setCmd(edit_->toPlainText());
}

void
UnixWidget::
rerunSlot()
{
  setCmd(cmdStr());
}

void
UnixWidget::
draw(QPainter *painter, int dx, int dy)
{
  if (! isError()) {
    TextWidget::draw(painter, dx, dy);
  }
  else {
    painter->fillRect(contentsRect(), errorColor_);

    painter->setPen(fgColor_);

    Widget::drawText(painter, dx, dy, errMsg_);
  }

  updateLayout();
}

QSize
UnixWidget::
contentsSizeHint() const
{
  QSize s1;

  if (! isError())
    s1 = TextWidget::contentsSizeHint();
  else
    s1 = TextWidget::textSize(errMsg_);

  return s1;
}

QSize
UnixWidget::
contentsSize() const
{
  QSize s1;

  if (! isError())
    s1 = TextWidget::contentsSize();
  else
    s1 = TextWidget::textSize(errMsg_);

  if (isEditing()) {
    auto s2 = TextWidget::textSize(this->cmd()) + QSize(32, 8);
  //auto s2 = edit_->document()->documentLayout()->documentSize() + QSizeF(2, 2);
    auto s3 = runButton_->sizeHint();

    return QSize(std::max(s1.width(), int(s2.width()) + s3.width()),
                 s1.height() + int(s2.height()));
  }
  else {
    return s1;
  }
}

}
