#ifndef CQDataFrameUnix_H
#define CQDataFrameUnix_H

#include <CQDataFrameText.h>

namespace CQDataFrame {

// unix command
class UnixWidget : public TextWidget {
  Q_OBJECT

  Q_PROPERTY(QString command READ cmd WRITE setCmd)

 public:
  using Args = std::vector<std::string>;

 public:
  UnixWidget(Area *area, const QString &cmd, const Args &args, const QString &res);

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
  Args    args_;
  QString errMsg_;
};

}

#endif
