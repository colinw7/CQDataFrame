#ifndef CQDataFrameHistory_H
#define CQDataFrameHistory_H

#include <CQDataFrameText.h>

namespace CQDataFrame {

// history text
// TODO: timestamp
class HistoryWidget : public Widget {
  Q_OBJECT

 public:
  HistoryWidget(Area *area, int ind, const QString &text="");

  QString id() const override;

  void save(QTextStream &) override;

  QSize calcSize() const override;

 private:
  bool canCollapse() const override { return false; }

  bool canDock() const override { return false; }

  bool canClose() const override { return false; }

  void draw(QPainter *painter) override;

  void drawText(QPainter *painter, int x, int y, const QString &text);

 private:
  int     ind_ { 0 };
  QString text_;
};

}

#endif
