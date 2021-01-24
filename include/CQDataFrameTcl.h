#ifndef CQDataFrameTcl_H
#define CQDataFrameTcl_H

#include <CQDataFrameText.h>

namespace CQDataFrame {

// tcl command
class TclWidget : public TextWidget {
  Q_OBJECT

 public:
  TclWidget(Area *area, const QString &cmd, const QString &res);

  //! get/set command
  const QString &cmd() const { return cmd_; }
  void setCmd(const QString &s) { cmd_ = s; }

  void addMenuItems(QMenu *menu) override;

  QSize calcSize() const override;

 private slots:
  void rerunSlot();

 private:
  void draw(QPainter *painter) override;

 private:
  QString cmd_;
};

}

#endif
