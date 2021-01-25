#include <CQDataFrameTcl.h>
#include <CQDataFrame.h>
#include <CQPixmapCache.h>

#include <QTextEdit>
#include <QToolButton>
//#include <QAbstractTextDocumentLayout>
#include <QMenu>
#include <QPainter>

#include <svg/run_svg.h>

namespace CQDataFrame {

TclWidget::
TclWidget(Area *area, const QString &cmd, const QString &res) :
 TextWidget(area, res), cmd_(cmd)
{
  setObjectName("tcl");

  //---

  edit_ = new QTextEdit(this);

  edit_->setObjectName("edit");
  edit_->setText(cmd_);
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
TclWidget::
setEditing(bool b)
{
  edit_     ->setVisible(b);
  runButton_->setVisible(b);

  TextWidget::setEditing(b);

  updateLayout();
}

void
TclWidget::
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

void
TclWidget::
setCmd(const QString &s)
{
  cmd_ = s;

  QString res;

  bool rc = runTclCommand(cmd_, res);

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

  frame()->larea()->placeWidgets();

  updateLayout();
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
draw(QPainter *painter)
{
  return TextWidget::draw(painter);
}

QSize
TclWidget::
calcSize() const
{
  if (isEditing()) {
    const auto &margins = contentsMargins();

    int xm = margins.left() + margins.right ();
    int ym = margins.top () + margins.bottom();

    //---

    auto s1 = TextWidget::textSize(this->text());
    auto s2 = TextWidget::textSize(this->cmd ()) + QSize(32, 8);
  //auto s2 = edit_->document()->documentLayout()->documentSize() + QSizeF(2, 2);
    auto s3 = runButton_->sizeHint();

    return QSize(std::max(s1.width(), int(s2.width()) + s3.width()) + xm,
                 s1.height() + int(s2.height()) + ym);
  }
  else {
    return TextWidget::calcSize();
  }
}

}
