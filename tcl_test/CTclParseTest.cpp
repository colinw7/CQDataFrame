#include <CTclParse.h>
#include <CArgs.h>
#include <CReadLine.h>

class CTclParseTest {
 public:
  CTclParseTest();

  bool isDebug() const { return debug_; }
  void setDebug(bool b);

  bool isTokens() const { return tokens_; }
  void setTokens(bool b) { tokens_ = b; }

  bool isTest() const { return test_; }
  void setTest(bool b) { test_ = b; }

  void parseFile(const std::string &filename);

  void mainLoop();

 private:
  void printSource(const CTclToken::Tokens &tokens);

  void printTokens(const CTclToken::Tokens &tokens);
  void printToken(CTclToken *token, int depth=0);

  void printTestTokens(const CTclToken::Tokens &tokens);

  bool readLine(std::string &line);

 private:
  bool       debug_    { false };
  bool       tokens_   { false };
  bool       test_     { false };
  CTclParse *tcl_      { nullptr };
  CReadLine *readline_ { nullptr };
};

//---

static std::string opts = "\
-tokens:f \
-debug:f \
-test:f \
";

int
main(int argc, char **argv)
{
  CArgs cargs(opts);

  cargs.parse(&argc, argv);

  bool debug  = false;
  bool tokens = false;
  bool test   = false;

  if (cargs.getBooleanArg("-debug"))
    debug = true;

  if (cargs.getBooleanArg("-tokens"))
    tokens = true;

  if (cargs.getBooleanArg("-test"))
    test = true;

  CTclParseTest parseTest;

  parseTest.setDebug (debug);
  parseTest.setTokens(tokens);
  parseTest.setTest  (test);

  if (argc > 1)
    parseTest.parseFile(argv[1]);
  else
    parseTest.mainLoop();

  return 0;
}

//---

CTclParseTest::
CTclParseTest()
{
  tcl_ = new CTclParse;
}

void
CTclParseTest::
setDebug(bool debug)
{
  debug_ = debug;

  tcl_->setDebug(debug_);
}

void
CTclParseTest::
parseFile(const std::string &filename)
{
  CTclToken::Tokens tokens;

  tcl_->parseFile(filename, tokens);

  if (isTest()) {
    enum class State {
      PRE_TEST,
      IN_TEST,
      POST_TEST,
    };

    State state = State::PRE_TEST;

    CTclToken::Tokens allTokens, testTokens, preTestTokens, inTestTokens, postTestTokens;

    bool in_test = false;

    for (auto *token : tokens) {
      if (token->type() == CTclToken::Type::COMMAND) {
        if      (token->str() == "beginTest") {
          state = State::IN_TEST;

          allTokens.push_back(token);

          continue;
        }
        else if (token->str() == "endTest") {
          state = State::POST_TEST;

          continue;
        }
        else if (token->str() == "test") {
          in_test = true;

          allTokens .push_back(token);
          testTokens.push_back(token);

          continue;
        }
        else
          in_test = false;
      }

      if      (in_test) {
        allTokens .push_back(token);
        testTokens.push_back(token);
      }
      else if (state == State::PRE_TEST) {
        preTestTokens.push_back(token);
        allTokens    .push_back(token);
      }
      else if (state == State::IN_TEST) {
        inTestTokens.push_back(token);
        allTokens   .push_back(token);
      }
      else if (state == State::POST_TEST) {
        postTestTokens.push_back(token);
      }
    }

    printSource(allTokens);

    std::cout << "\n";
    std::cout << "stop_tool\n";
    std::cout << "start_tool\n";
    std::cout << "\n";

    printTestTokens(testTokens);

    std::cout << "\n";
    std::cout << "endTest\n";

    printTestTokens(postTestTokens);
  }
  else {
    if (isTokens())
      printTokens(tokens);
    else
      printSource(tokens);
  }
}

void
CTclParseTest::
printTokens(const CTclToken::Tokens &tokens)
{
  for (const auto &token : tokens)
    printToken(token);
}

void
CTclParseTest::
printToken(CTclToken *token, int depth)
{
  for (int i = 0; i < depth; ++i)
    std::cout << "  ";

  token->print(std::cout, /*children*/false);

  std::cout << "\n";

  for (const auto &token1 : token->tokens()) {
    printToken(token1, depth + 1);
  }
}

void
CTclParseTest::
printSource(const CTclToken::Tokens &tokens)
{
  int currentLineNum = 1;
  int currentLinePos = 0;

  for (const auto &token : tokens) {
    int lineNum = token->lineNum();
    int linePos = token->linePos();

    while (lineNum > currentLineNum) {
      std::cout << "\n";

      ++currentLineNum;

      currentLinePos = 0;
    }

    while (linePos > currentLinePos) {
      std::cout << " ";

      ++currentLinePos;
    }

    const auto &str = token->str();

    for (std::size_t i = 0; i < str.size(); ++i) {
      auto &c = str[i];

      if (c == '\n') {
        ++currentLineNum;

        currentLinePos = 0;
      }
      else
        ++currentLinePos;

      std::cout << c;
    }
  }

  if (currentLinePos > 0)
    std::cout << "\n";
}

void
CTclParseTest::
printTestTokens(const CTclToken::Tokens &tokens)
{
  int lineNum = -1;
  int charNum = 0;

  for (const auto &token : tokens) {
    if (lineNum < 0)
      lineNum = token->lineNum();

    while (token->lineNum() > lineNum) {
      std::cout << "\n";

      ++lineNum;

      charNum = 0;
    }

    while (token->linePos() > charNum) {
      std::cout << " ";

      ++charNum;
    }

    std::cout << token->str();

    charNum += token->str().size();
  }

  if (charNum > 0)
    std::cout << "\n";
}

void
CTclParseTest::
mainLoop()
{
  readline_ = new CReadLine;

  std::string line;

  while (readLine(line)) {
    CTclToken::Tokens tokens;

    if (tcl_->parseLine(line, tokens)) {
      for (const auto &token : tokens) {
        token->print(std::cout);

        std::cout << "\n";
      }
    }
  }

  delete readline_;
}

bool
CTclParseTest::
readLine(std::string &line)
{
  readline_->setPrompt("% ");

  std::string line1 = readline_->readLine();

  if (readline_->eof()) return false;

  while (! tcl_->isCompleteLine(line1)) {
    readline_->setPrompt("");

    std::string line2 = readline_->readLine();

    if (readline_->eof()) return false;

    line1 += "\n" + line2;
  }

  line = line1;

  return true;
}
