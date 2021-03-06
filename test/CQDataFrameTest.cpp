#include <CQDataFrameTest.h>
#include <CQDataFrame.h>

#include <CQDataFrameImage.h>
#include <CQDataFrameCanvas.h>
#ifdef MARKDOWN_DATA
#include <CQDataFrameMarkdown.h>
#endif
#ifdef FILE_MGR_DATA
#include <CQDataFrameFileMgr.h>
#endif
#include <CQDataFrameWeb.h>
#include <CQDataFrameSVG.h>
#ifdef FILE_DATA
#include <CQDataFrameFile.h>
#endif
#include <CQDataFrameHtml.h>

#define USE_CQAPP 1

#ifdef USE_CQAPP
#include <CQApp.h>
#else
#include <QApplication>
#endif

#include <QVBoxLayout>

CQDataFrameTest::
CQDataFrameTest() :
 QFrame()
{
  setObjectName("dataFrameTest");

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  frame_ = new CQDataFrame::Frame(this);

  frame_->init();

  addWidgets();

  layout->addWidget(frame_);
}

void
CQDataFrameTest::
addWidgets()
{
  frame_->addWidgetFactory(new CQDataFrame::ImageFactory );
  frame_->addWidgetFactory(new CQDataFrame::CanvasFactory);

#ifdef MARKDOWN_DATA
  frame_->addWidgetFactory(new CQDataFrame::MarkdownFactory);
#endif

#ifdef FILE_MGR_DATA
  frame_->addWidgetFactory(new CQDataFrame::FileMgrFactory);
#endif

  frame_->addWidgetFactory(new CQDataFrame::WebFactory );
  frame_->addWidgetFactory(new CQDataFrame::SVGFactory );
#ifdef FILE_DATA
  frame_->addWidgetFactory(new CQDataFrame::FileFactory);
#endif
  frame_->addWidgetFactory(new CQDataFrame::HtmlFactory);
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
#ifdef USE_CQAPP
  CQApp app(argc, argv);
#else
  QApplication app(argc, argv);
#endif

  app.setFont(QFont("Sans", 20));

  auto *test = new CQDataFrameTest;

  test->resize(2000, 1500);

  test->show();

  if (argc > 1)
    test->load(argv[1]);

  return app.exec();
}
