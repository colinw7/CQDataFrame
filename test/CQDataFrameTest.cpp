#include <CQDataFrameTest.h>
#include <CQDataFrame.h>

#include <QApplication>
#include <QVBoxLayout>

CQDataFrameTest::
CQDataFrameTest() :
 QFrame()
{
  setObjectName("dataFrameTest");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  frame_ = new CQDataFrame::Frame(this);

  layout->addWidget(frame_);
}

void
CQDataFrameTest::
load(const QString &fileName)
{
  frame_->load(fileName);
}

//------

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  app.setFont(QFont("Sans", 20));

  auto *test = new CQDataFrameTest;

  test->resize(2000, 1500);

  test->show();

  if (argc > 1)
    test->load(argv[1]);

  return app.exec();
}
