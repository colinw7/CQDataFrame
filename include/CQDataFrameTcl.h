#ifndef CQDataFrameTcl_H
#define CQDataFrameTcl_H

#include <CQDataFrameText.h>

class CQIconButton;
class QTextEdit;

namespace CQDataFrame {

// tcl command
class TclWidget : public TextWidget {
  Q_OBJECT

  Q_PROPERTY(QString command READ cmd WRITE setCmd)

 public:
  TclWidget(Area *area, const QString &cmd, bool expr, const QString &res);

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
  void updateLayout();

 private slots:
  void textChangedSlot();

  void runCmdSlot();

  void rerunSlot();

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  QString       cmd_;
  bool          expr_      { false };
  QTextEdit*    edit_      { nullptr };
  CQIconButton* runButton_ { nullptr };
};

}

#endif
