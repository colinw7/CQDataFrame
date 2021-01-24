#ifndef CQDataFrameWeb_H
#define CQDataFrameWeb_H

#include <CQDataFrame.h>

class QWebView;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Web)

// web
class WebWidget : public Widget {
  Q_OBJECT

 public:
  WebWidget(Area *area, const QString &addr="");

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

class WebFactory : public WidgetFactory {
 public:
  WebFactory() { }

  const char *name() const override { return "web"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("web", new WebTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    auto *widget = new WebWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif
