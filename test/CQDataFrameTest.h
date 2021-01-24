#include <QFrame>

namespace CQDataFrame { class Frame; }

//---

class CQDataFrameTest : public QFrame {
  Q_OBJECT

 public:
  CQDataFrameTest();

  void addWidgets();

  void load(const QString &fileName);

 private:
  CQDataFrame::Frame *frame_ { nullptr };
};
