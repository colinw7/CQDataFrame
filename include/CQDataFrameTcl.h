#ifndef CQDataFrameTcl_H
#define CQDataFrameTcl_H

#include <CQDataFrameText.h>

class QTextEdit;
class QToolButton;

namespace CQDataFrame {

// tcl command
class TclWidget : public TextWidget {
  Q_OBJECT

  Q_PROPERTY(QString command READ cmd WRITE setCmd)

 public:
  TclWidget(Area *area, const QString &cmd, const QString &res);

  bool canEdit() const override { return true; }

  void setEditing(bool b) override;

  //! get/set command
  const QString &cmd() const { return cmd_; }
  void setCmd(const QString &s);

  void addMenuItems(QMenu *menu) override;

  QSize calcSize() const override;

 private:
  void updateLayout();

 private slots:
  void textChangedSlot();

  void runCmdSlot();

  void rerunSlot();

 private:
  void draw(QPainter *painter) override;

 private:
  QString      cmd_;
  QTextEdit*   edit_      { nullptr };
  QToolButton* runButton_ { nullptr };
};

}

#endif
