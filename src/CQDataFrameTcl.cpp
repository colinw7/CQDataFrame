#include <CQDataFrameTcl.h>
#include <CQDataFrame.h>
#include <CQIconButton.h>

#include <QTextEdit>
//#include <QAbstractTextDocumentLayout>
#include <QMenu>
#include <QPainter>

#include <svg/run_svg.h>

namespace CQDataFrame {

TclWidget::
TclWidget(Area *area, const QString &cmd, bool expr, const QString &res) :
 TextWidget(area, res), cmd_(cmd), expr_(expr)
{
  setObjectName("tcl");
}

void
TclWidget::
addWidgets()
{
  edit_ = new QTextEdit(this);

  edit_->setObjectName("edit");
  edit_->setText(cmd_);
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
TclWidget::
setEditing(bool b)
{
  TextWidget::setEditing(b);

  updateLayout();

  placeWidgets();
}

void
TclWidget::
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

void
TclWidget::
setCmd(const QString &s)
{
  cmd_ = s;

  QString res;

  auto cmd1 = (expr_ ? "expr {" + cmd_ + "}" : cmd_);

  bool rc = runTclCommand(cmd1, res);

  setText(res);

  setIsError(! rc);

  if (cmd_ != edit_->toPlainText())
    edit_->setText(cmd_);

  emit contentsChanged();
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
textChangedSlot()
{
  cmd_ = edit_->toPlainText();

  // update layout to ensure command widget is not scrolled/clipped
  updateLayout();

  placeWidgets();

  //update();
}

void
TclWidget::
runCmdSlot()
{
  setCmd(edit_->toPlainText());
}

void
TclWidget::
rerunSlot()
{
  setCmd(cmd_);
}

void
TclWidget::
draw(QPainter *painter, int dx, int dy)
{
  TextWidget::draw(painter, dx, dy);

  updateLayout();
}

QSize
TclWidget::
contentsSizeHint() const
{
  auto s1 = TextWidget::contentsSizeHint();

  if (isEditing()) {
    auto s2 = TextWidget::textSize(this->cmd()) + QSize(48, 8);
    auto s3 = runButton_->sizeHint();

    return QSize(std::max(s1.width(), int(s2.width()) + s3.width()), s1.height());
  }
  else {
    return s1;
  }
}

QSize
TclWidget::
contentsSize() const
{
  auto s1 = TextWidget::contentsSize();

  if (isEditing()) {
    auto s2 = TextWidget::textSize(this->cmd()) + QSize(48, 8);
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
