#ifndef CQDataFrame_H
#define CQDataFrame_H

#include <CQScrollArea.h>
#include <CQTclUtil.h>

#include <QFrame>
#include <QVariant>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>

namespace CQTclCmd { class Mgr; }

class CQTabSplit;

class QTextStream;

//------

namespace CQDataFrame {

class Scroll;
class Area;
class Status;
class TclCmdProc;

class Widget;

class CommandWidget;
class TextWidget;
class HistoryWidget;

class WidgetFactory;

//------

// container widget for data frame widgets (cells and status bar)
class Frame : public QFrame {
  Q_OBJECT

 public:
  using Vars = std::vector<QVariant>;

 public:
  static QStringList s_completeFile(const QString &file);

  static bool s_fileToLines(const QString &fileName, QStringList &lines);

  static bool s_stringToBool(const QString &str, bool *ok);

 public:
  Frame(QWidget *parent=nullptr);
 ~Frame();

  Scroll *lscroll() const { return lscroll_; }
  Scroll *rscroll() const { return rscroll_; }

  Status *status() const { return status_; }

  Area *larea() const;
  Area *rarea() const;

  //---

  void init();

  //---

  void addWidgetFactory(WidgetFactory *factory);
  WidgetFactory *getWidgetFactory(const QString &name) const;

  //---

  CQTcl *qtcl() const { return qtcl_; }

  CQTclCmd::Mgr *tclCmdMgr() const { return mgr_; }

  //---

  Widget *getWidget(const QString &id) const;

  //--

  bool setCmdRc(int rc);
  bool setCmdRc(double rc);
  bool setCmdRc(const QString &str);
  bool setCmdRc(const QVariant &rc);
  bool setCmdRc(const QStringList &rc);

  //---

  bool help(const QString &pattern, bool verbose, bool hidden);

  void helpAll(bool verbose, bool hidden);

  //---

  void save();

  bool load();
  bool load(const QString &fileName);

  //---

  QSize sizeHint() const override;

 private:
  using WidgetFactories = std::map<QString, WidgetFactory *>;

 private:
  CQTabSplit* tab_     { nullptr };
  Scroll*     lscroll_ { nullptr };
  Scroll*     rscroll_ { nullptr };
  Status*     status_  { nullptr };
  CQTcl*      qtcl_    { nullptr };

  CQTclCmd::Mgr *mgr_ { nullptr };

  WidgetFactories widgetFactories_;
};

//---

// scrolled area for data frame widgets
class Scroll : public CQScrollArea {
  Q_OBJECT

 public:
  Scroll(Frame *frame, bool isCommand);

  Frame *frame() const { return frame_; }

  Area *area() const { return area_; }

  bool isCommand() const { return isCommand_; }
  void setIsCommand(bool b) { isCommand_ = b; }

  void updateContents() override;

 private:
  Frame* frame_     { nullptr };
  Area*  area_      { nullptr };
  bool   isCommand_ { false };
};

//---

// main container area for data frame widgets
class Area : public QFrame {
  Q_OBJECT

 public:
  using Args = std::vector<std::string>;

 public:
  Area(Scroll *scroll);
 ~Area();

  Scroll *scroll() const { return scroll_; }

  Frame *frame() const { return scroll()->frame(); }

  const QString &prompt() const { return prompt_; }
  void setPrompt(const QString &s);

  CommandWidget *commandWidget() const;

  int margin() const { return margin_; }
  void setMargin(int margin) { margin_ = margin; }

  //---

  TextWidget    *addTextWidget   (const QString &text);
  HistoryWidget *addHistoryWidget(const QString &text);

  //---

  void scrollToEnd();

  void moveToEnd(Widget *widget);

  void addWidget   (Widget *widget);
  void removeWidget(Widget *widget);

  Widget *getWidget(const QString &id) const;

  //---

  int xOffset() const;
  int yOffset() const;

  void placeWidgets();

  //---

  void save(QTextStream &);
  void load();

 private slots:
  void updateWidgets();

 private:
  void resizeEvent(QResizeEvent *) override;

  bool event(QEvent *event) override;

 private:
  using Widgets = std::vector<Widget *>;

  Scroll*        scroll_       { nullptr };
  QString        prompt_       { "> " };
  Widgets        widgets_;
  CommandWidget* command_      { nullptr };
  int            ind_          { 0 };
  int            margin_       { 4 };
};

//---

class Status : public QFrame {
  Q_OBJECT

 public:
  Status(Frame *frame);

  int margin() const { return margin_; }

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  Frame* frame_  { nullptr };
  int    margin_ { 2 };
};

}

//------

namespace CQDataFrame {

class WidgetFactory {
 public:
  WidgetFactory() { }

  virtual ~WidgetFactory() { }

  virtual const char *name() const = 0;

  virtual void addTclCommand(Frame *frame) = 0;

  virtual Widget *addWidget(Area *area) = 0;
};

//---

template <typename T, typename... TArg>
static T *makeWidget(Area *area, TArg&&... Args) {
  auto *widget = new T(area, std::forward<TArg>(Args)...);
  widget->init();
  area->addWidget(widget);
  return widget;
}

}

//------

#include <CQDataFrameTclCmd.h>

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Help)
CQDATA_FRAME_TCL_CMD(Complete)

//---

CQDATA_FRAME_TCL_CMD(GetData)
CQDATA_FRAME_TCL_CMD(SetData)

}

#endif
