#ifndef CQDataFrameCommand_H
#define CQDataFrameCommand_H

#include <CQDataFrameText.h>

namespace CQDataFrame {

// command entry
class CommandWidget : public Widget {
  Q_OBJECT

 public:
  using Args = std::vector<std::string>;

 public:
  CommandWidget(Area *area);

  QString id() const override;

  bool canMove  () const override { return false; }
  bool canResize() const override { return false; }

  bool isCompleteLine(const QString &line, bool &isTcl) const;

  bool runUnixCommand(const std::string &cmd, const Args &args, QString &res) const;

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

}

#endif
