#include <CQDataFrameUnix.h>
#include <CQDataFrame.h>

#include <QTextEdit>
#include <QToolButton>
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

  //---

  edit_ = new QTextEdit(this);

  edit_->setObjectName("edit");
  edit_->setText(cmdStr());
  edit_->setVisible(false);

  runButton_ = new QToolButton(this);
  runButton_->setIcon(CQPixmapCacheInst->getIcon("RUN"));
  runButton_->setIconSize(QSize(32, 32));
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
  edit_     ->setVisible(b);
  runButton_->setVisible(b);

  TextWidget::setEditing(b);

  updateLayout();
}

void
UnixWidget::
updateLayout()
{
  edit_     ->setVisible(isEditing());
  runButton_->setVisible(isEditing());

  if (isEditing()) {
    int th = TextWidget::textSize(this->text()).height();
    int bw = runButton_->sizeHint().width();

    edit_->move(margin_, margin_ + th);

    edit_->resize(width() - 2*margin_ - bw, height() - 2*margin_ - th);

    runButton_->move(edit_->x() + edit_->width(), edit_->y());

    edit_     ->raise();
    runButton_->raise();
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

  // parse command
  std::string              name;
  std::vector<std::string> args;

  parseCommand(s, name, args);

  cmd_  = name.c_str();
  args_ = args;

  frame()->larea()->placeWidgets();

  updateLayout();
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

    Widget::drawText(painter, x, y, errMsg_);
  }
}

QSize
UnixWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  QSize s1;

  if (! isError())
    s1 = TextWidget::textSize(this->text());
  else
    s1 = TextWidget::textSize(errMsg_);

  if (isEditing()) {
    auto s2 = TextWidget::textSize(this->cmd ()) + QSize(32, 8);
  //auto s2 = edit_->document()->documentLayout()->documentSize() + QSizeF(2, 2);
    auto s3 = runButton_->sizeHint();

    return QSize(std::max(s1.width(), int(s2.width()) + s3.width()) + xm,
                 s1.height() + int(s2.height()) + ym);
  }
  else {
    return QSize(s1.width() + xm, s1.height() + ym);
  }
}

}
