#ifndef CQDataFrameWidget_H
#define CQDataFrameWidget_H

#include <CQScrollArea.h>
#include <QFrame>

class CQTcl;

class QMenu;
class QTextStream;

namespace CQDataFrame {

class Area;
class Frame;
class WidgetContents;

class Widget : public QFrame {
  Q_OBJECT

  Q_PROPERTY(int contentsWidth  READ width  WRITE setContentsWidth )
  Q_PROPERTY(int contentsHeight READ height WRITE setContentsHeight)

 public:
  using Args = std::vector<std::string>;

 public:
  Widget(Area *area);

  //---

  void init();

  virtual void addWidgets() { }

  //---

  Area *area() const { return area_; }
  void setArea(Area *area) { area_ = area; }

  Area *larea() const;
  Area *rarea() const;

  Frame *frame() const;

  CQTcl *qtcl() const;

  virtual QString id() const = 0;

  int pos() const { return pos_; }
  void setPos(int i) { pos_ = i; }

  bool isExpanded() const { return expanded_; }
  virtual void setExpanded(bool b);

  bool isEditing() const { return editing_; }
  bool isDocked() const { return docked_; }

  bool isResizeFrame   () const { return resizeFrame_   ; }
  bool isResizeContents() const { return resizeContents_; }

  int x() const { return x_; }
  virtual void setX(int x) { x_ = x; }

  int y() const { return y_; }
  virtual void setY(int y) { y_ = y; }

  void setWidgetWidth (int w);
  void setWidgetHeight(int h);

  virtual int contentsWidth() const { return width_; }
  virtual void setContentsWidth(int w);

  virtual int contentsHeight() const { return height_; }
  virtual void setContentsHeight(int h);

  virtual int defaultWidth () const { return 100; }
  virtual int defaultHeight() const { return 100; }

  virtual bool getNameValue(const QString &name, QVariant &value) const;
  virtual bool setNameValue(const QString &name, const QVariant &value);

  virtual void addMenuItems(QMenu *menu);

  void placeWidgets();

  void updateSize();

  virtual void draw(QPainter *painter, int dx, int dy) = 0;

  virtual void handleResize(int /*w*/, int /*h*/) { }

  void drawText(QPainter *painter, int x, int y, const QString &text);

  virtual void save(QTextStream &) { }

  //---

  bool runTclCommand(const QString &line, QString &res) const;

  bool runUnixCommand(const std::string &cmd, const Args &args, QString &res) const;

  void parseCommand(const QString &line, std::string &name, Args &args) const;

  //---

  void paintEvent(QPaintEvent *e) override;

  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  //---

  void contentsMousePress  (QMouseEvent *e);
  void contentsMouseMove   (QMouseEvent *e);
  void contentsMouseRelease(QMouseEvent *e);

  bool pixelToText(const QPoint &p, int &lineNum, int &charNum);

  void drawContents(QPainter *painter);

//void resizeContents(int w, int h);

  virtual QSize contentsSizeHint() const = 0;
  virtual QSize contentsSize() const = 0;

  QSize sizeHint() const override;

 protected:
  void setFixedFont();

 Q_SIGNALS:
  void contentsChanged();

 protected Q_SLOTS:
  void contentsUpdateSlot();

  void setExpandedSlot(bool b) { setExpanded(b); }

  virtual void setEditing(bool b);
  virtual void setDocked(bool b);

  virtual void setResizeFrame   (bool b);
  virtual void setResizeContents(bool b);

  void copySlot();
  void pasteSlot();
  void closeSlot();
  void loadSlot();
  void saveSlot();

 protected:
  class Line {
   public:
    Line() = default;

    Line(const QString &text) : text_(text) { }

    const QString &text() const { return text_; }
    void setText(const QString &s) { text_ = s; }

    int x() const { return x_; }
    void setX(int x) { x_ = x; }

    int y() const { return y_; }
    void setY(int y) { y_ = y; }

    bool isJoin() const { return join_; }
    void setJoin(bool b) { join_ = b; }

   private:
    QString text_;
    int     x_    { 0 };
    int     y_    { 0 };
    bool    join_ { false };
  };

  using LineList = std::vector<Line *>;

 protected:
  virtual bool canCollapse() const { return true; }

  virtual bool canDock() const { return true; }

  virtual bool canMove  () const { return true; }
  virtual bool canResize() const { return true; }

  virtual bool canEdit() const { return false; }

  virtual bool canClose() const { return true; }

  void contextMenuEvent(QContextMenuEvent *e) override;

  void drawBorder(QPainter *painter) const;

  QRect contentsRect() const;

  virtual void copy () { }
  virtual void paste();

  virtual void pasteText(const QString &) { }

  void copyText(const QString &text) const;

 protected:
  struct CharData {
    int width   { 10 };
    int height  { 12 };
    int ascent  { 12 };
  //int descent { 0 };
  };

  void updateFont();

 protected:
  struct MouseData {
    bool pressed      { false };
    int  pressLineNum { -1 };
    int  pressCharNum { -1 };
    int  moveLineNum  { -1 };
    int  moveCharNum  { -1 };

    bool dragging { false };
    bool resizing { false };
    bool resizeX  { true };
    bool resizeY  { true };
    int  dragX    { 0 };
    int  dragY    { 0 };
    int  dragW    { 0 };
    int  dragH    { 0 };
    int  dragX1   { 0 };
    int  dragY1   { 0 };
    int  dragX2   { 0 };
    int  dragY2   { 0 };

    void reset() {
      pressed = false;

      dragging = false;
      resizing = false;
      resizeX  = true;
      resizeY  = true;
      dragX    = 0;
      dragY    = 0;
      dragW    = 0;
      dragH    = 0;
      dragX1   = 0;
      dragY1   = 0;
      dragX2   = 0;
      dragY2   = 0;
    }

    void resetSelection() {
      pressLineNum = -1;
      pressCharNum = -1;
      moveLineNum  = -1;
      moveCharNum  = -1;
    }
  };

  using ScrollArea = CQScrollArea;
  using Contents   = WidgetContents;

  Area*       area_           { nullptr }; //!< parent area
  ScrollArea* scrollArea_     { nullptr }; //!< scroll area
  Contents*   contents_       { nullptr }; //!< scrolled contents
  int         pos_            { -1 };
  bool        expanded_       { true };
  bool        editing_        { false };
  bool        docked_         { false };
  bool        resizeFrame_    { true };
  bool        resizeContents_ { true };
  QColor      bgColor_        { 240, 240, 240 };
  QColor      fgColor_        { 0, 0, 0 };
  CharData    charData_;
  int         margin_         { 2 };
  int         x_              { 0 };
  int         y_              { 0 };
  int         width_          { -1 };
  int         height_         { -1 };
  LineList    lines_;
  MouseData   mouseData_;
};

//---

class WidgetContents : public QFrame {
  Q_OBJECT

 public:
  WidgetContents(Widget *widget);

  //---

  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  void paintEvent(QPaintEvent *e) override;

  void resizeEvent(QResizeEvent *e) override;

  //---

  QSize sizeHint() const override;

  QSize contentsSize() const;

 private:
  Widget* widget_ { nullptr };
};

}

#endif
