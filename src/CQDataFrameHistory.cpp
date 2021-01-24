#include <CQDataFrameHistory.h>

#include <QPainter>
#include <QTextStream>

namespace CQDataFrame {

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

}
