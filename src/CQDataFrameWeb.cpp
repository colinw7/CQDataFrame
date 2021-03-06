#include <CQDataFrameWeb.h>

#ifdef USE_WEBENGINE
#include <QWebEngineView>
#include <QWebEngineFrame>
#else
#include <QWebView>
#include <QWebFrame>
#endif

#include <QVBoxLayout>

namespace CQDataFrame {

WebWidget::
WebWidget(Area *area, const QString &addr) :
 Widget(area), addr_(addr)
{
  setObjectName("web");
}

void
WebWidget::
addWidgets()
{
  auto *layout = new QVBoxLayout(contents_);
  layout->setMargin(0); layout->setSpacing(0);

#ifdef USE_WEBENGINE
  web_ = new QWebEngineView;
#else
  web_ = new QWebView;
#endif

  web_->load(QUrl(addr_));

  layout->addWidget(web_);
}

QString
WebWidget::
id() const
{
  return QString("web.%1").arg(pos());
}

void
WebWidget::
draw(QPainter *, int /*dx*/, int /*dy*/)
{
}

QSize
WebWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
WebWidget::
contentsSize() const
{
  return QSize(width_, height_);
}

//------

void
WebTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-addr", ArgType::String, "web address");
}

QStringList
WebTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
WebTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto addr = argv.getParseStr("addr");

  auto *area = frame_->larea();

  auto *widget = makeWidget<WebWidget>(area, addr);

  return frame_->setCmdRc(widget->id());
}

//------

}
