#ifndef CQDataFrameWeb_H
#define CQDataFrameWeb_H

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>

class QWebView;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Web)

// web
class WebWidget : public Widget {
  Q_OBJECT

 public:
  WebWidget(Area *area, const QString &addr="");

  void addWidgets() override;

  QString id() const override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private:
  void draw(QPainter *painter, int dx, int dy) override;

 private:
  QString addr_;

#ifdef USE_WEBENGINE
  QWebEngineView *web_ { nullptr };
#else
  QWebView*       web_ { nullptr };
#endif
};

class WebFactory : public WidgetFactory {
 public:
  WebFactory() { }

  const char *name() const override { return "web"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("web", new WebTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<WebWidget>(area);
  }
};

}

#endif
