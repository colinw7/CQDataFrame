#ifndef CQDataFrame_H
#define CQDataFrame_H

#include <CQScrollArea.h>
#include <CQTclUtil.h>

#include <CDisplayRange2D.h>

#include <QFrame>
#include <QImage>
#include <QVariant>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>

class CQTabSplit;

class CQDataModel;
class CQModelDetails;

class CQTcl;

class QTextStream;
class QMenu;
class QWebView;
class QTextEdit;

//---

namespace CQDataFrame {

class Scroll;
class Area;
class Status;
class TclCmdProc;

class Widget;

class CommandWidget;
class TextWidget;
class HistoryWidget;
class UnixWidget;
class TclWidget;

class ImageWidget;
class CanvasWidget;
class WebWidget;
class MarkdownWidget;
class HtmlWidget;
class SVGWidget;

//---

struct FileText {
  enum class Type {
    FILENAME,
    TEXT
  };

  FileText() = default;

  FileText(Type type, const QString &value) :
   type(type), value(value) {
  }

  Type    type { Type::TEXT };
  QString value;
};

//---

// container widget for data frame widgets (cells and status bar)
class Frame : public QFrame {
  Q_OBJECT

 public:
  using Vars = std::vector<QVariant>;

 public:
  Frame(QWidget *parent=nullptr);
 ~Frame();

  Scroll *lscroll() const { return lscroll_; }
  Scroll *rscroll() const { return rscroll_; }

  Status *status() const { return status_; }

  //---

  CQTcl *qtcl() const { return qtcl_; }

  void addTclCommand(const QString &name, TclCmdProc *proc);

  bool processTclCmd(const QString &name, const Vars &vars);

  bool setCmdRc(int rc);
  bool setCmdRc(double rc);
  bool setCmdRc(const QString &str);
  bool setCmdRc(const QVariant &rc);
  bool setCmdRc(const QStringList &rc);

  //---

  void save();

  bool load();
  bool load(const QString &fileName);

  //---

  QString addModel(CQDataModel *model);

  CQDataModel *getModel(const QString &name) const;

  CQModelDetails *getModelDetails(const QString &name) const;

  //---

  CanvasWidget *canvas() const { return canvas_; }
  void setCanvas(CanvasWidget *canvas) { canvas_ = canvas; }

 private:
  struct ModelData {
    QString         id;
    CQDataModel*    model   { nullptr };
    CQModelDetails* details { nullptr };
  };

  using CommandNames = std::vector<QString>;
  using CommandProcs = std::map<QString, TclCmdProc *>;
  using NamedModel   = std::map<QString, ModelData>;

  CQTabSplit*   tab_     { nullptr };
  Scroll*       lscroll_ { nullptr };
  Scroll*       rscroll_ { nullptr };
  Status*       status_  { nullptr };
  CQTcl*        qtcl_    { nullptr };
  CommandNames  commandNames_;
  CommandProcs  commandProcs_;

  NamedModel namedModel_;

  CanvasWidget *canvas_ { nullptr };
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

  void updateContents();

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

  const QString &prompt() const { return prompt_; }
  void setPrompt(const QString &s);

  CommandWidget *commandWidget() const;

  //---

  CommandWidget *addCommandWidget();

  TextWidget    *addTextWidget   (const QString &text);
  HistoryWidget *addHistoryWidget(const QString &text);
  UnixWidget    *addUnixWidget   (const QString &cmd, const Args &args, const QString &res);
  TclWidget     *addTclWidget    (const QString &line, const QString &res);

  ImageWidget    *addImageWidget   (const QString &name);
  CanvasWidget   *addCanvasWidget  (int width, int height);
  WebWidget      *addWebWidget     (const QString &addr);
  MarkdownWidget *addMarkdownWidget(const FileText &fileText);
  HtmlWidget     *addHtmlWidget    (const FileText &fileText);
  SVGWidget      *addSVGWidget     (const FileText &fileText);

  //---

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

class Widget : public QFrame {
  Q_OBJECT

 public:
  Widget(Area *area);

  Area *area() const { return area_; }

  virtual QString id() const = 0;

  int pos() const { return pos_; }
  void setPos(int i) { pos_ = i; }

  bool isExpanded() const { return expanded_; }

  bool isDocked() const { return docked_; }

  int x() const { return x_; }
  void setX(int x) { x_ = x; }

  int y() const { return y_; }
  void setY(int y) { y_ = y; }

  virtual bool getNameValue(const QString & /*name*/, QVariant & /*value*/) const { return false; }
  virtual bool setNameValue(const QString & /*name*/, const QVariant & /*value*/) { return false; }

  virtual void addMenuItems(QMenu *menu);

  virtual void draw(QPainter *painter) = 0;

  virtual void handleResize(int /*w*/, int /*h*/) { }

  void drawText(QPainter *painter, int x, int y, const QString &text);

  virtual void save(QTextStream &) { }

  QSize sizeHint() const override;

  virtual QSize calcSize() const = 0;

 protected:
  void setFixedFont();

 signals:
  void contentsChanged();

 protected slots:
  void setExpanded(bool b);
  void setDocked(bool b);

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

  virtual bool canClose() const { return true; }

  void contextMenuEvent(QContextMenuEvent *e) override;

  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  void pixelToText(const QPoint &p, int &lineNum, int &charNum);

  void paintEvent(QPaintEvent *e) override;

  void resizeEvent(QResizeEvent *) override;

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
    int  dragX    { 0 };
    int  dragY    { 0 };
    int  dragX1   { 0 };
    int  dragY1   { 0 };
    int  dragX2   { 0 };
    int  dragY2   { 0 };

    void reset() {
      pressed = false;

      dragging = false;
      dragX    = 0;
      dragY    = 0;
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

  Area*     area_     { nullptr };
  int       pos_      { -1 };
  bool      expanded_ { true };
  bool      docked_   { false };
  QColor    bgColor_  { 240, 240, 240 };
  QColor    fgColor_  { 0, 0, 0 };
  CharData  charData_;
  int       margin_   { 2 };
  int       x_        { 0 };
  int       y_        { 0 };
  LineList  lines_;
  MouseData mouseData_;
};

//---

// raw text
class TextWidget : public Widget {
  Q_OBJECT

 public:
  TextWidget(Area *area, const QString &text="");

  QString id() const override;

  const QString &text() const { return text_; }
  void setText(const QString &text);

  QSize calcSize() const override;

  bool isError() const { return isError_; }
  void setIsError(bool b) { isError_ = b; }

 protected:
  void updateLines();

  void draw(QPainter *painter) override;

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

//---

// unix command
class UnixWidget : public TextWidget {
  Q_OBJECT

 public:
  using Args = std::vector<std::string>;

 public:
  UnixWidget(Area *area, const QString &cmd, const Args &args, const QString &res);

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

//---

// unix command
class TclWidget : public TextWidget {
  Q_OBJECT

 public:
  TclWidget(Area *area, const QString &line, const QString &res);

  void addMenuItems(QMenu *menu) override;

  QSize calcSize() const override;

 private slots:
  void rerunSlot();

 private:
  void draw(QPainter *painter) override;

 private:
  QString line_;
};

//---

// history text
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

//---

// command entry
class CommandWidget : public Widget {
  Q_OBJECT

 public:
  using Args = std::vector<std::string>;

 public:
  CommandWidget(Area *area);

  QString id() const override;

  bool isCompleteLine(const QString &line, bool &isTcl) const;

  bool runUnixCommand(const std::string &cmd, const Args &args, std::string &res) const;

  bool runTclCommand(const QString &line, QString &res) const;

  void processCommand(const QString &line);

  QSize calcSize() const override;

 private:
  class Entry {
   public:
    Entry(const QString &text="") :
     text_(text) {
    }

    const QString &getText() const { return text_; }
    void setText(const QString &text) { text_ = text; pos_ = 0; }

    int getPos() const { return pos_; }

    void clear() { setText(""); }

    void cursorStart() { pos_ = 0; }
    void cursorEnd  () { pos_ = text_.length(); }

    void cursorLeft() {
      if (pos_ > 0)
        --pos_;
    }

    void cursorRight() {
      if (pos_ < text_.length())
        ++pos_;
    }

    void insert(const QString &str) {
      auto lhs = text_.mid(0, pos_);
      auto rhs = text_.mid(pos_);

      text_ = lhs + str + rhs;

      pos_ += str.length();
    }

    void backSpace() {
      if (pos_ > 0) {
         auto lhs = text_.mid(0, pos_);
         auto rhs = text_.mid(pos_);

        text_ = lhs.mid(0, lhs.length() - 1) + rhs;

        --pos_;
      }
    }

    void deleteChar() {
      if (pos_ < text_.length()) {
        auto lhs = text_.mid(0, pos_);
        auto rhs = text_.mid(pos_);

        text_ = lhs + rhs.mid(1);
      }
    }

   private:
    QString text_;
    int     pos_ { 0 };
  };

  //---

  enum class CompleteMode {
    None,
    Longest,
    Interactive
  };

  //---

  void parseCommand(const QString &line, std::string &name, Args &args) const;

  bool complete(const QString &text, int pos, QString &newText, CompleteMode completeMode) const;

  //---

  bool canCollapse() const override { return false; }

  bool canClose() const override { return false; }

  void draw(QPainter *painter) override;

  QString getText() const;

  void clearEntry();

  void clearText();

  //---

  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  void pixelToText(const QPoint &p, int &lineNum, int &charNum);

  //---

  bool event(QEvent *e) override;

  void keyPressEvent(QKeyEvent *e) override;

  //---

  void drawCursor(QPainter *painter, int x, int y, const QChar &c);

  void drawLine(QPainter *painter, Line *line, int y);

  void drawSelectedChars(QPainter *painter, int lineNum1, int charNum1,
                         int lineNum2, int charNum2);

  void drawText(QPainter *painter, int x, int y, const QString &text);

  bool hasFocus() const { return true; }

  void copy() override;

  void pasteText(const QString &text) override;

  QString selectedText() const;

 private:
  Entry       entry_;
  QStringList commands_;
  int         commandNum_   { -1 };
  QColor      cursorColor_  { 60, 217, 60 };
  QColor      selColor_     { 217, 217, 8 };
  int         promptY_      { 0 };
  int         promptWidth_  { 0 };
};

//---

// image
class ImageWidget : public Widget {
  Q_OBJECT

  Q_PROPERTY(QString file   READ file   WRITE setFile  )
  Q_PROPERTY(int     width  READ width  WRITE setWidth )
  Q_PROPERTY(int     height READ height WRITE setHeight)

 public:
  ImageWidget(Area *area, const QString &file);

  QString id() const override;

  const QString &file() const { return file_; }
  void setFile(const QString &s);

  int width() const { return width_; }
  void setWidth(int i);

  int height() const { return height_; }
  void setHeight(int i);

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  void pasteText(const QString &) override;

  QSize calcSize() const override;

 private:
  void updateImage();

  void draw(QPainter *painter) override;

 private:
  QString file_;
  QImage  image_;
  int     width_  { -1 };
  int     height_ { -1 };
  QImage  simage_;
};

//---

// image
class CanvasWidget : public Widget {
  Q_OBJECT

  Q_PROPERTY(int width  READ width  WRITE setWidth )
  Q_PROPERTY(int height READ height WRITE setHeight)

 public:
  CanvasWidget(Area *area, int width, int height);

  QString id() const override;

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  int width() const { return width_; }
  void setWidth(int i) { width_ = i; }

  int height() const { return height_; }
  void setHeight(int i) { height_ = i; }

  const QString &drawProc() const { return drawProc_; }
  void setDrawProc(const QString &s) { drawProc_ = s; }

  QSize calcSize() const override;

  void setBrush(const QBrush &brush);

  void setPen(const QPen &pen);

  void drawPath(const QString &path);

 private:
  void handleResize(int w, int h) override;

  void draw(QPainter *painter) override;

 private:
  using DisplayRange = CDisplayRange2D;

  int          width_   { 100 };
  int          height_  { 100 };
  QString      drawProc_;
  DisplayRange displayRange_;
  QPainter*    painter_ { nullptr };
};

//---

// web
class WebWidget : public Widget {
  Q_OBJECT

 public:
  WebWidget(Area *area, const QString &addr);

  QString id() const override;

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private:
  QString addr_;

#ifdef USE_WEBENGINE
  QWebEngineView *web_ { nullptr };
#else
  QWebView*       web_ { nullptr };
#endif
};

//---

// markdown
class MarkdownWidget : public Widget {
  Q_OBJECT

 public:
  MarkdownWidget(Area *area, const FileText &fileText);

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private:
  FileText   fileText_;
  QTextEdit* textEdit_ { nullptr };
};

//---

// html
class HtmlWidget : public Widget {
  Q_OBJECT

 public:
  HtmlWidget(Area *area, const FileText &fileText);

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private:
  FileText   fileText_;
  QTextEdit* textEdit_ { nullptr };
};

//---

// svg
class SVGWidget : public Widget {
  Q_OBJECT

 public:
  SVGWidget(Area *area, const FileText &fileText);

  QString id() const override;

  const FileText &fileText() const { return fileText_; }
  void setFileText(const FileText &fileText);

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  QSize calcSize() const override;

 private:
  void draw(QPainter *painter) override;

 private:
  FileText   fileText_;
  QTextEdit *textEdit_ { nullptr };
  int        width_    { -1 };
  int        height_   { -1 };
};

//---

class Status : public QFrame {
  Q_OBJECT

 public:
  Status(Frame *frame);

  void paintEvent(QPaintEvent *);

  QSize sizeHint() const;

 private:
  Frame* frame_  { nullptr };
  int    margin_ { 2 };
};

//---

class TclCmdArg {
 public:
  //! types
  enum class Type {
    None,
    Boolean,
    Integer,
    Real,
    String,
    SBool
  };

  using NameValue  = std::pair<QString, int>;
  using NameValues = std::vector<NameValue>;

 public:
  TclCmdArg(int ind, const QString &name, Type type, const QString &argDesc="",
            const QString &desc="") :
   ind_(ind), name_(name), type_(type), argDesc_(argDesc), desc_(desc) {
    if (name_.left(1) == "-") {
      isOpt_ = true;

      name_ = name_.mid(1);
    }
  }

  int ind() const { return ind_; }

  const QString &name() const { return name_; }

  bool isOpt() const { return isOpt_; }

  Type type() const { return type_; }

  const QString &argDesc() const { return argDesc_; }

  const QString &desc() const { return desc_; }

  bool isRequired() const { return required_; }
  TclCmdArg &setRequired(bool b=true) { required_ = b; return *this; }

  bool isHidden() const { return hidden_; }
  TclCmdArg &setHidden(bool b=true) { hidden_ = b; return *this; }

  bool isMultiple() const { return multiple_; }
  TclCmdArg &setMultiple(bool b=true) { multiple_ = b; return *this; }

  int groupInd() const { return groupInd_; }
  void setGroupInd(int i) { groupInd_ = i; }

  TclCmdArg &addNameValue(const QString &name, int value) {
    nameValues_.push_back(NameValue(name, value));
    return *this;
  }

  const NameValues &nameValues() const { return nameValues_; }

 private:
  int        ind_      { -1 };         //!< command ind
  QString    name_;                    //!< arg name
  bool       isOpt_    { false };      //!< is option
  Type       type_     { Type::None }; //!< value type
  QString    argDesc_;                 //!< short description
  QString    desc_;                    //!< long description
  bool       required_ { false };      //!< is required
  bool       hidden_   { false };      //!< is hidden
  bool       multiple_ { false };      //!< can have multiple values
  int        groupInd_ { -1 };         //!< cmd group ind
  NameValues nameValues_;              //!< enum name values
};

class TclCmdGroup {
 public:
  enum class Type {
    None,
    OneOpt,
    OneReq
  };

 public:
  TclCmdGroup(int ind, Type type) :
   ind_(ind), type_(type) {
  }

  int ind() const { return ind_; }

  bool isRequired() const { return (type_ == Type::OneReq); }

 private:
  int  ind_ { -1 };
  Type type_ { Type::None };
};

//---

class TclCmdBaseArgs {
 public:
  using CmdGroup = TclCmdGroup;
  using CmdArg   = TclCmdArg;
  using Args     = std::vector<QVariant>;

  class Arg {
   public:
    Arg(const QVariant &var=QVariant()) :
     var_(var) {
      QString varStr = toString(var_);

      isOpt_ = (varStr.length() && varStr[0] == '-');
    }

  //QString  str() const { assert(! isOpt_); return toString(var_); }
    QVariant var() const { assert(! isOpt_); return var_; }

    bool isOpt() const { return isOpt_; }
    void setIsOpt(bool b) { isOpt_ = b; }

    QString opt() const { assert(isOpt_); return toString(var_).mid(1); }

   private:
    QVariant var_;             //!< arg value
    bool     isOpt_ { false }; //!< is option
  };

  //---

 public:
  TclCmdBaseArgs(const QString &cmdName, const Args &argv) :
   cmdName_(cmdName), argv_(argv), argc_(argv_.size()) {
  }

  // add argument
  CmdArg &addCmdArg(const QString &name, CmdArg::Type type,
                    const QString &argDesc="", const QString &desc="") {
    int ind = cmdArgs_.size() + 1;

    cmdArgs_.emplace_back(ind, name, type, argDesc, desc);

    CmdArg &cmdArg = cmdArgs_.back();

    cmdArg.setGroupInd(groupInd_);

    return cmdArg;
  }

  //---

  // has processed last argument
  bool eof() const { return (i_ >= argc_); }

  // get next arg
  const Arg &getArg() {
    assert(i_ < argc_);

    lastArg_ = Arg(argv_[i_++]);

    return lastArg_;
  }

  //---

  // get string or string list value of current option
  bool getOptValue(QStringList &strs) {
    if (eof()) return false;

    strs = toStringList(argv_[i_++]);

    return true;
  }

  // get string value of current option
  bool getOptValue(QString &str) {
    if (eof()) return false;

    str = toString(argv_[i_++]);

    return true;
  }

  // get integer value of current option
  bool getOptValue(int &i) {
    QString str;

    if (! getOptValue(str))
      return false;

    bool ok;

    i = str.toInt(&ok);

    return ok;
  }

  // get real value of current option
  bool getOptValue(double &r) {
    QString str;

    if (! getOptValue(str))
      return false;

    bool ok;

    r = str.toDouble(&ok);

    return ok;
  }

  // get optional boolean value of current option
  bool getOptValue(bool &b) {
    QString str;

    if (! getOptValue(str))
      return false;

    str = str.toLower();

    bool ok;

    b = stringToBool(str, &ok);

    return ok;
  }

  //---

  // get/set debug
  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  //---

  // parse command arguments
  bool parse() {
    bool rc;

    return parse(rc);
  }

  bool parse(bool &rc) {
    rc = false;

    // clear parsed values
    parseInt_ .clear();
    parseReal_.clear();
    parseStr_ .clear();
    parseBool_.clear();
    parseArgs_.clear();

    //---

    using Names      = std::set<QString>;
    using GroupNames = std::map<int, Names>;

    GroupNames groupNames;

    //---

    bool help       = false;
    bool showHidden = false;
    bool allowOpt   = true;

    while (! eof()) {
      // get next arg
      Arg arg = getArg();

      if (! allowOpt && arg.isOpt())
        arg.setIsOpt(false);

      // handle option (starts with '-')
      if (arg.isOpt()) {
        // get option name
        QString opt = arg.opt();

        //---

        // flag if help option
        if (opt == "help") {
          help = true;
          continue;
        }

        // flag if hidden option (don't skip argument for command)
        if (opt == "hidden") {
          showHidden = true;
        }

        // Thandle '--' for no more options
        if (opt == "-") {
          allowOpt = false;
          continue;
        }

        //---

        // get arg data for option (flag error if not found)
        CmdArg *cmdArg = getCmdOpt(opt);

        if (! cmdArg) {
          if (opt != "hidden")
            return this->error();
        }

        //---

        // record option for associated group (if any)
        if (cmdArg->groupInd() >= 0) {
          groupNames[cmdArg->groupInd()].insert(cmdArg->name());
        }

        //---

        // handle bool option (no value)
        if      (cmdArg->type() == CmdArg::Type::Boolean) {
          parseBool_[opt] = true;
        }
        // handle integer option
        else if (cmdArg->type() == CmdArg::Type::Integer) {
          int i = 0;

          if (getOptValue(i)) {
            parseInt_[opt] = i;
          }
          else {
            return valueError(opt);
          }
        }
        // handle real option
        else if (cmdArg->type() == CmdArg::Type::Real) {
          double r = 0.0;

          if (getOptValue(r)) {
            parseReal_[opt] = r;
          }
          else {
            return valueError(opt);
          }
        }
        // handle string option
        else if (cmdArg->type() == CmdArg::Type::String) {
          if (cmdArg->isMultiple()) {
            QStringList strs;

            if (getOptValue(strs)) {
              for (int i = 0; i < strs.length(); ++i)
                parseStr_[opt].push_back(strs[i]);
            }
            else
              return valueError(opt);
          }
          else {
            QString str;

            if (getOptValue(str))
              parseStr_[opt].push_back(str);
            else
              return valueError(opt);
          }
        }
        // handle string bool option
        else if (cmdArg->type() == CmdArg::Type::SBool) {
          bool b;

          if (getOptValue(b)) {
            parseBool_[opt] = b;
          }
          else {
            return valueError(opt);
          }
        }
        // invalid type (assert ?)
        else {
          std::cerr << "Invalid type for '" << opt.toStdString() << "'\n";
          continue;
        }
      }
      // handle argument (no '-')
      else {
        // save argument
        parseArgs_.push_back(arg.var());
      }
    }

    //---

    // if help option specified ignore other options and process help
    if (help) {
      this->help(showHidden);

      if (! isDebug()) {
        rc = true;

        return false;
      }
    }

    //---

    // check options specified for cmd groups
    for (const auto &cmdGroup : cmdGroups_) {
      int groupInd = cmdGroup.ind();

      auto p = groupNames.find(groupInd);

      // handle no options for required group
      if (p == groupNames.end()) {
        if (cmdGroup.isRequired()) {
          std::string names = getGroupArgNames(groupInd).join(", ").toStdString();
          std::cerr << "One of " << names << " required\n";
          return false;
        }
      }
      // handle multiple options for one of group
      else {
        const Names &names = (*p).second;

        if (names.size() > 1) {
          std::string names = getGroupArgNames(groupInd).join(", ").toStdString();
          std::cerr << "Only one of " << names << " allowed\n";
          return false;
        }
      }
    }

    //---

    // display parsed data for debug
    if (isDebug()) {
      for (auto &pi : parseInt_) {
        std::cerr << pi.first.toStdString() << "=" << pi.second << "\n";
      }
      for (auto &pr : parseReal_) {
        std::cerr << pr.first.toStdString() << "=" << pr.second << "\n";
      }
      for (auto &ps : parseStr_) {
        const QString     &name   = ps.first;
        const QStringList &values = ps.second;

        for (int i = 0; i < ps.second.length(); ++i) {
          std::cerr << name.toStdString() << "=" << values[i].toStdString() << "\n";
        }
      }
      for (auto &ps : parseBool_) {
        std::cerr << ps.first.toStdString() << "=" << ps.second << "\n";
      }
      for (auto &a : parseArgs_) {
        std::cerr << toString(a).toStdString() << "\n";
      }
    }

    //---

    rc = true;

    return true;
  }

  //---

  // check if option found by parse
  bool hasParseArg(const QString &name) const {
    if      (parseInt_ .find(name) != parseInt_ .end()) return true;
    else if (parseReal_.find(name) != parseReal_.end()) return true;
    else if (parseStr_ .find(name) != parseStr_ .end()) return true;
    else if (parseBool_.find(name) != parseBool_.end()) return true;

    return false;
  }

  //---

  // get parsed int for option (default returned if not found)
  int getParseInt(const QString &name, int def=0) const {
    auto p = parseInt_.find(name);
    if (p == parseInt_.end()) return def;

    return (*p).second;
  }

  // get parsed real for option (default returned if not found)
  double getParseReal(const QString &name, double def=0.0) const {
    auto p = parseReal_.find(name);
    if (p == parseReal_.end()) return def;

    return (*p).second;
  }

  // get parsed string for option (if multiple return first) (default returned if not found)
  QString getParseStr(const QString &name, const QString &def="") const {
    auto p = parseStr_.find(name);
    if (p == parseStr_.end()) return def;

    return (*p).second[0];
  }

  // get parsed boolean for option (default returned if not found)
  bool getParseBool(const QString &name, bool def=false) const {
    auto p = parseBool_.find(name);
    if (p == parseBool_.end()) return def;

    return (*p).second;
  }

  //---

  // get arg data for option
  CmdArg *getCmdOpt(const QString &name) {
    for (auto &cmdArg : cmdArgs_) {
      if (cmdArg.isOpt() && cmdArg.name() == name)
        return &cmdArg;
    }

    return nullptr;
  }

  //---

  // handle missing value error
  bool valueError(const QString &opt) {
    errorMsg(QString("Missing value for '-%1'").arg(opt));
    return false;
  }

  // handle invalid option/arg error
  bool error() {
    if (lastArg_.isOpt())
      errorMsg("Invalid option '" + lastArg_.opt() + "'");
    else
      errorMsg("Invalid arg '" + toString(lastArg_.var()) + "'");
    return false;
  }

  //---

  // display help
  void help(bool showHidden=false) const {
    using GroupIds = std::set<int>;

    GroupIds groupInds;

    std::cerr << cmdName_.toStdString() << "\n";

    for (auto &cmdArg : cmdArgs_) {
      if (! showHidden && cmdArg.isHidden())
        continue;

      int groupInd = cmdArg.groupInd();

      if (groupInd > 0) {
        auto p = groupInds.find(groupInd);

        if (p == groupInds.end()) {
          std::cerr << "  ";

          helpGroup(groupInd, showHidden);

          std::cerr << "\n";

          groupInds.insert(groupInd);
        }
        else
          continue;
      }
      else {
        std::cerr << "  ";

        if (! cmdArg.isRequired())
          std::cerr << "[";

        helpArg(cmdArg);

        if (! cmdArg.isRequired())
          std::cerr << "]";

        std::cerr << "\n";
      }
    }

    std::cerr << "  [-help]\n";
  }

  // display help for group
  void helpGroup(int groupInd, bool showHidden) const {
    const CmdGroup &cmdGroup = cmdGroups_[groupInd - 1];

    if (! cmdGroup.isRequired())
      std::cerr << "[";

    CmdArgs cmdArgs;

    getGroupCmdArgs(groupInd, cmdArgs);

    int i = 0;

    for (const auto &cmdArg : cmdArgs) {
      if (! showHidden && cmdArg.isHidden())
        continue;

      if (i > 0)
        std::cerr << "|";

      helpArg(cmdArg);

      ++i;
    }

    if (! cmdGroup.isRequired())
      std::cerr << "]";
  }

  // display help for arg
  void helpArg(const CmdArg &cmdArg) const {
    if (cmdArg.isOpt()) {
      std::cerr << "-" << cmdArg.name().toStdString() << " ";

      if (cmdArg.type() != CmdArg::Type::Boolean)
        std::cerr << "<" << cmdArg.argDesc().toStdString() << ">";
    }
    else {
      std::cerr << "<" << cmdArg.argDesc().toStdString() << ">";
    }
  }

  //---

  // variant to string list
  static QStringList toStringList(const QVariant &var) {
    QStringList strs;

    if (var.type() == QVariant::List) {
      QList<QVariant> vars = var.toList();

      for (int i = 0; i < vars.length(); ++i) {
        QString str = toString(vars[i]);

        strs.push_back(str);
      }
    }
    else {
      strs.push_back(var.toString());
    }

    return strs;
  }

  //---

  // variant to string
  static QString toString(const QVariant &var) {
    if (var.type() == QVariant::List) {
      QList<QVariant> vars = var.toList();

      QStringList strs;

      for (int i = 0; i < vars.length(); ++i) {
        QString str = toString(vars[i]);

        strs.push_back(str);
      }

      CQTcl tcl;

      return tcl.mergeList(strs);
    }
    else
      return var.toString();
  }

  // string to bool
  static bool stringToBool(const QString &str, bool *ok) {
    QString lstr = str.toLower();

    if (lstr == "0" || lstr == "false" || lstr == "no") {
      *ok = true;
      return false;
    }

    if (lstr == "1" || lstr == "true" || lstr == "yes") {
      *ok = true;
      return true;
    }

    *ok = false;

    return false;
  }

 protected:
  using CmdArgs     = std::vector<CmdArg>;
  using CmdGroups   = std::vector<CmdGroup>;
  using NameInt     = std::map<QString, int>;
  using NameReal    = std::map<QString, double>;
  using NameString  = std::map<QString, QString>;
  using NameStrings = std::map<QString, QStringList>;
  using NameBool    = std::map<QString, bool>;

 protected:
  // get option names for group
  QStringList getGroupArgNames(int groupInd) const {
    CmdArgs cmdArgs;

    getGroupCmdArgs(groupInd, cmdArgs);

    QStringList names;

    for (const auto &cmdArg : cmdArgs)
      names << cmdArg.name();

    return names;
  }

  // get option datas for group
  void getGroupCmdArgs(int groupInd, CmdArgs &cmdArgs) const {
    for (auto &cmdArg : cmdArgs_) {
      int groupInd1 = cmdArg.groupInd();

      if (groupInd1 != groupInd)
        continue;

      cmdArgs.push_back(cmdArg);
    }
  }

 protected:
  // display error message
  void errorMsg(const QString &msg) {
    std::cerr << msg.toStdString() << "\n";
  }

 protected:
  QString     cmdName_;             //! command name being processed
  bool        debug_     { false }; //! is debug
  Args        argv_;                //! input args
  int         i_         { 0 };     //! current arg
  int         argc_      { 0 };     //! number of args
  Arg         lastArg_;             //! last processed arg
  CmdArgs     cmdArgs_;             //! command argument data
  CmdGroups   cmdGroups_;           //! command argument groups
  int         groupInd_  { -1 };    //! current group index
  NameInt     parseInt_;            //! parsed option integers
  NameReal    parseReal_;           //! parsed option reals
  NameStrings parseStr_;            //! parsed option strings
  NameBool    parseBool_;           //! parsed option booleans
  Args        parseArgs_;           //! parsed arguments
};

//---

class TclCmdArgs : public TclCmdBaseArgs {
 public:
  TclCmdArgs(const QString &cmdName, const Args &argv) :
   TclCmdBaseArgs(cmdName, argv) {
  }
};

//---

class TclCmd;

class TclCmdProc {
 public:
  using Vars         = std::vector<QVariant>;
  using NameValueMap = std::map<QString, QString>;

 public:
  TclCmdProc(Frame *frame) :
   frame_(frame) {
  }

  virtual ~TclCmdProc() { }

  Frame *frame() const { return frame_; }

  const QString &name() const { return name_; }
  void setName(const QString &v) { name_ = v; }

  TclCmd *tclCmd() const { return tclCmd_; }
  void setTclCmd(TclCmd *tclCmd) { tclCmd_ = tclCmd; }

  virtual bool exec(TclCmdArgs &args) = 0;

  virtual void addArgs(TclCmdArgs & /*args*/) { }

  virtual QStringList getArgValues(const QString& /*arg*/, const NameValueMap& /*nameValueMap*/) {
    return QStringList();
  }

 protected:
  Frame*  frame_  { nullptr };
  QString name_;
  TclCmd* tclCmd_ { nullptr };
};

//---

#define CQDATA_DEF_TCL_CMD(NAME) \
class Tcl##NAME##Cmd : public TclCmdProc { \
 public: \
  Tcl##NAME##Cmd(Frame *frame) : TclCmdProc(frame) { } \
\
  bool exec(TclCmdArgs &args) override; \
\
  void addArgs(TclCmdArgs &args) override; \
\
  QStringList getArgValues(const QString &arg, \
                           const NameValueMap &nameValueMap=NameValueMap()) override; \
};

CQDATA_DEF_TCL_CMD(Image)
CQDATA_DEF_TCL_CMD(Canvas)
CQDATA_DEF_TCL_CMD(Web)
CQDATA_DEF_TCL_CMD(Markdown)
CQDATA_DEF_TCL_CMD(Html)
CQDATA_DEF_TCL_CMD(SVG)
CQDATA_DEF_TCL_CMD(Model)

CQDATA_DEF_TCL_CMD(GetData)
CQDATA_DEF_TCL_CMD(SetData)

CQDATA_DEF_TCL_CMD(GetModelData)
CQDATA_DEF_TCL_CMD(ShowModel)

//---

class TclCanvasInstCmd : public TclCmdProc {
 public:
  TclCanvasInstCmd(Frame *frame, const QString &id) :
   TclCmdProc(frame), id_(id) {
  }

  bool exec(TclCmdArgs &args) override;

  void addArgs(TclCmdArgs &args) override;

  QStringList getArgValues(const QString &arg,
                           const NameValueMap &nameValueMap=NameValueMap()) override;

 private:
  QString id_;
};

}

#endif
