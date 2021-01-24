#include <CQDataFrameUnix.h>
#include <CQDataFrame.h>

#include <QMenu>
#include <QPainter>

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

  QString res;

  if (command->runUnixCommand(cmd_.toStdString(), args_, res))
    text_ = res;
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

}
