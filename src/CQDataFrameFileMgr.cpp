#include <CQDataFrameFileMgr.h>
#include <CQDataFrameImage.h>

#ifdef FILE_MGR_DATA

#include <CQFileBrowser.h>
#include <CQDirView.h>
#include <CFileBrowser.h>
#include <CFileUtil.h>
#include <CFile.h>

#include <QVBoxLayout>

namespace CQDataFrame {

void
FileMgrWidget::
s_init()
{
  static bool initialized;

  if (! initialized) {
    CQDirViewFactory::init();

    initialized = true;
  }
}

FileMgrWidget::
FileMgrWidget(Area *area) :
 Widget(area)
{
  s_init();

  setObjectName("filemgr");
}

void
FileMgrWidget::
addWidgets()
{
  auto *layout = new QVBoxLayout(contents_);
  layout->setMargin(0); layout->setSpacing(0);

  fileMgr_ = new CQFileBrowser;

  fileMgr_->getFileBrowser()->setShowDotDot(false);
  fileMgr_->getFileBrowser()->setFontSize(16);

  connect(fileMgr_, SIGNAL(fileActivated(const QString &)),
          this, SLOT(fileActivated(const QString &)));

  layout->addWidget(fileMgr_);
}

QString
FileMgrWidget::
id() const
{
  return QString("filemgr.%1").arg(pos());
}

bool
FileMgrWidget::
getNameValue(const QString &name, QVariant &value) const
{
  return Widget::getNameValue(name, value);
}

bool
FileMgrWidget::
setNameValue(const QString &name, const QVariant &value)
{
  return Widget::setNameValue(name, value);
}

void
FileMgrWidget::
draw(QPainter *, int /*dx*/, int /*dy*/)
{
}

QSize
FileMgrWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
FileMgrWidget::
contentsSize() const
{
  return QSize(width_, height_);
}

void
FileMgrWidget::
fileActivated(const QString &filename)
{
  auto name = filename.toStdString();

  CFile file(name);

  CFileType type = CFileUtil::getType(&file);

  if (type & CFILE_TYPE_IMAGE) {
    auto *frame = this->frame();

    auto *factory = frame->getWidgetFactory("image");

    if (factory) {
      auto *widget = qobject_cast<ImageWidget *>(factory->addWidget(larea()));

      if (widget)
        widget->setFile(filename);
    }
  }
}

//------

void
FileMgrTclCmd::
addArgs(CQTclCmd::CmdArgs &)
{
}

QStringList
FileMgrTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
FileMgrTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto *area = frame_->larea();

  auto *widget = makeWidget<FileMgrWidget>(area);

  return frame_->setCmdRc(widget->id());
}

}

#endif
