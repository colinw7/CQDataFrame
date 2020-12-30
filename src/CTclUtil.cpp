#include <CTclUtil.h>
#include <CStrParse.h>
#include <vector>

class CTclUtilP {
 public:
  CTclUtilP() { }

  bool isCompleteLine(char endChar);

  void startStringParse(const std::string &str);
  void endParse();

 private:
  using ParseStack = std::vector<CStrParse *>;

  CStrParse* parse_ { nullptr };
  ParseStack parseStack_;
};

//---

bool
CTclUtil::
isCompleteLine(const std::string &line)
{
  CTclUtilP util;

  util.startStringParse(line);

  bool rc = util.isCompleteLine('\0');

  util.endParse();

  return rc;
}

//---

void
CTclUtilP::
startStringParse(const std::string &str)
{
  parseStack_.push_back(parse_);

  parse_ = new CStrParse(str);
}

void
CTclUtilP::
endParse()
{
  delete parse_;

  parse_ = parseStack_.back();

  parseStack_.pop_back();
}

bool
CTclUtilP::
isCompleteLine(char endChar)
{
  while (! parse_->eof()) {
    if      (parse_->isChar('[')) {
      parse_->skipChar();

      if (! isCompleteLine(']'))
        return false;

      if (! parse_->isChar(']'))
        return false;

      parse_->skipChar();
    }
    else if (parse_->isChar('{')) {
      parse_->skipChar();

      if (! isCompleteLine('}'))
        return false;

      if (! parse_->isChar('}'))
        return false;

      parse_->skipChar();
    }
    else if (parse_->isChar(endChar)) {
      return true;
    }
    else if (parse_->isChar('\"')) {
      parse_->skipChar();

      if (! isCompleteLine('\"'))
        return false;

      if (! parse_->isChar('\"'))
        return false;

      parse_->skipChar();
    }
    else if (parse_->isChar('\'')) {
      parse_->skipChar();

      if (! isCompleteLine('\''))
        return false;

      if (! parse_->isChar('\''))
        return false;

      parse_->skipChar();
    }
    else if (parse_->isChar('\\')) {
      parse_->skipChar();
      parse_->skipChar();
    }
    else
      parse_->skipChar();
  }

  return true;
}
