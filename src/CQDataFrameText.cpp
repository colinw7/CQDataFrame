#include <CQDataFrameText.h>

#include <QPainter>

namespace CQDataFrame {

TextWidget::
TextWidget(Area *area, const QString &text) :
 Widget(area), text_(text)
{
  setObjectName("text");

  bgColor_ = QColor(204, 221, 255);

  //---

  setFixedFont();

  updateLines();
}

TextWidget::
~TextWidget()
{
  freeLines(lines_);
}

QString
TextWidget::
id() const
{
  return QString("text.%1").arg(pos());
}

void
TextWidget::
setText(const QString &text)
{
  text_ = text;

  updateLines();
}

void
TextWidget::
updateLines()
{
  calcLines(lines_, text());
}

void
TextWidget::
calcLines(LineList &lines, const QString &text) const
{
  freeLines(lines);

  //---

  int len = text.length();

  QString s;

  for (int i = 0; i < len; ++i) {
    if (text[i] == '\n') {
      lines.push_back(new Line(s));

      s = "";
    }
    else
      s += text[i];
  }

  if (s.length())
    lines.push_back(new Line(s));
}

void
TextWidget::
freeLines(LineList &lines) const
{
  for (auto &line : lines)
    delete line;

  lines.clear();
}

void
TextWidget::
draw(QPainter *painter)
{
  const auto &margins = contentsMargins();

  int x = margins.left();
  int y = margins.top ();

  //---

  if (isError())
    painter->fillRect(contentsRect(), errorColor_);

  //---

  drawText(painter, x, y);
}

void
TextWidget::
drawText(QPainter *painter, int x, int &y)
{
  // draw lines
  painter->setPen(fgColor_);

  for (const auto &line : lines_) {
    line->setX(x);
    line->setY(y);

    Widget::drawText(painter, x, y, line->text());

    y += charData_.height;
  }

  //---

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
TextWidget::
drawSelectedChars(QPainter *painter, int lineNum1, int charNum1, int lineNum2, int charNum2)
{
  int numLines = lines_.size();

  painter->setPen(bgColor_);

  for (int i = lineNum1; i <= lineNum2; ++i) {
    if (i < 0 || i >= numLines)
      continue;

    auto *line = lines_[i];

    int ty = line->y();
    int tx = line->x();

    const auto &text = line->text();

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

      Widget::drawText(painter, tx1, ty, text.mid(j, 1));
    }
  }
}

void
TextWidget::
copy()
{
  copyText(selectedText());
}

QString
TextWidget::
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

  int numLines = lines_.size();

  QString str;

  for (int i = lineNum1; i <= lineNum2; ++i) {
    if (i < 0 || i >= numLines) continue;

    auto *line = lines_[i];

    const auto &text = line->text();

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

QSize
TextWidget::
calcSize() const
{
  const auto &margins = contentsMargins();

  int xm = margins.left() + margins.right ();
  int ym = margins.top () + margins.bottom();

  //---

  const auto &text = this->text();

  QSize size = textSize(text);

  return QSize(size.width() + xm, size.height() + ym);
}

QSize
TextWidget::
textSize(const QString &text) const
{
  int len = text.length();

  int maxWidth     = 0;
  int currentWidth = 0;
  int numLines     = 0;

  for (int i = 0; i < len; ++i) {
    if (text[i] == '\n') {
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

  return QSize(maxWidth*charData_.width, numLines*charData_.height);
}

}
