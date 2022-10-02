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
draw(QPainter *painter, int dx, int dy)
{
  painter->setPen(fgColor_);

  drawText(painter, dx, dy, text_);
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
contentsSizeHint() const
{
  return calcSize(25);
}

QSize
HistoryWidget::
contentsSize() const
{
  return calcSize(-1);
}

QSize
HistoryWidget::
calcSize(int maxLines) const
{
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

  if (numLines < 1)
    numLines = 1;

  if (maxLines > 0 && numLines > maxLines)
    numLines = maxLines;

  return QSize(maxWidth*charData_.width, numLines*charData_.height);
}

}
