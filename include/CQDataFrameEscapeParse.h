#ifndef CQDataFrameEscapeParse_H
#define CQDataFrameEscapeParse_H

#ifdef ESCAPE_PARSER

#include <CEscapeParse.h>

namespace CQDataFrame {

class EscapeParse : public CEscapeParse {
 public:
  EscapeParse() { }

 ~EscapeParse() { }

  const std::string &str() const { return str_; }

  void handleChar(char c) override {
    str_ +=  c;
  }

  void handleGraphic(char) override {
  }

  void handleEscape(const CEscapeData *e) override {
    if      (e->type == CEscapeType::BS ) { }
    else if (e->type == CEscapeType::HT ) { str_ += '\t'; }
    else if (e->type == CEscapeType::LF ) { str_ += '\n'; }
    else if (e->type == CEscapeType::VT ) { str_ += '\v'; }
    else if (e->type == CEscapeType::FF ) { str_ += '\f'; }
    else if (e->type == CEscapeType::CR ) { str_ += '\r'; }
    else if (e->type == CEscapeType::SGR) {
    }
  }

  void log(const std::string &) const override { }

  void logError(const std::string &) const override { }

  std::string waitMessage(const char *, uint) { return ""; }

  std::string processString(const std::string &s) {
    for (std::size_t i = 0; i < s.size(); ++i)
      addOutputChar(s[i]);

    return str();
  }

 private:
  std::string str_;
};

}

#endif

#endif
