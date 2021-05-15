#ifndef CQDataFrameUnix_H
#define CQDataFrameUnix_H

#include <CQDataFrameText.h>

class CQIconButton;
class QTextEdit;

namespace CQDataFrame {

// unix command
class UnixWidget : public TextWidget {
  Q_OBJECT

  Q_PROPERTY(QString command READ cmd WRITE setCmd)

 public:
  using Args = std::vector<std::string>;

 public:
  UnixWidget(Area *area, const QString &cmd, const Args &args, const QString &res);

  void addWidgets() override;

  bool canEdit() const override { return true; }

  void setEditing(bool b) override;

  //! get/set command
  const QString &cmd() const { return cmd_; }
  void setCmd(const QString &s);

  void addMenuItems(QMenu *menu) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  QString cmdStr() const;

  void updateLayout();

 private slots:
  void textChangedSlot();

  void runCmdSlot();

  void rerunSlot();

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  QString       cmd_;
  Args          args_;
  QString       errMsg_;
  QTextEdit*    edit_      { nullptr };
  CQIconButton* runButton_ { nullptr };
};

}

#endif
