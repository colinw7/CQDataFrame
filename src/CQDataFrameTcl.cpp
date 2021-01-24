#include <CQDataFrameTcl.h>
#include <CQDataFrame.h>

#include <QMenu>

namespace CQDataFrame {

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

}
