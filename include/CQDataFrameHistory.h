#ifndef CQDataFrameHistory_H
#define CQDataFrameHistory_H

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>

namespace CQDataFrame {

// history text
// TODO: timestamp
class HistoryWidget : public Widget {
  Q_OBJECT

 public:
  HistoryWidget(Area *area, int ind, const QString &text="");

  QString id() const override;

  void save(QTextStream &) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

  QSize calcSize(int maxLines) const;

 private:
  bool canCollapse() const override { return false; }

  bool canDock() const override { return false; }

  bool canClose() const override { return false; }

  void draw(QPainter *painter, int dx, int dy) override;

  void drawText(QPainter *painter, int x, int y, const QString &text);

 private:
  int     ind_ { 0 };
  QString text_;
};

}

#endif
