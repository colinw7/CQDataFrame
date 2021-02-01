#ifndef CQDataFrameText_H
#define CQDataFrameText_H

#include <CQDataFrameWidget.h>

namespace CQDataFrame {

// raw text
class TextWidget : public Widget {
  Q_OBJECT

  Q_PROPERTY(QString text    READ text    WRITE setText   )
  Q_PROPERTY(bool    isError READ isError WRITE setIsError)

 public:
  TextWidget(Area *area, const QString &text="");

  virtual ~TextWidget();

  QString id() const override;

  //! get/set text
  const QString &text() const { return text_; }
  void setText(const QString &text);

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

  QSize textSize(const QString &text, int maxLines=-1) const;

  //! get/set is error
  bool isError() const { return isError_; }
  void setIsError(bool b) { isError_ = b; }

 protected:
  void updateLines();

  void calcLines(LineList &lines, const QString &text) const;
  void freeLines(LineList &lines) const;

  void draw(QPainter *painter, int dx, int dy) override;

  void drawText(QPainter *painter, int x, int &y);

  void drawSelectedChars(QPainter *painter, int lineNum1, int charNum1,
                         int lineNum2, int charNum2);

  void copy() override;

  QString selectedText() const;

 protected:
  QString text_;
  bool    isError_    { false };
  QColor  errorColor_ { 255, 204, 204 };
  QColor  selColor_   { 217, 217, 8 };
};

}

#endif
