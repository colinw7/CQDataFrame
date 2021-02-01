#include <CQDataFrameFile.h>

#ifdef FILE_DATA
//#include <CQEdit.h>
#include <CQVi.h>

#include <QVBoxLayout>

namespace CQDataFrame {

void
FileWidget::
s_init()
{
  static bool initialized;

  if (! initialized) {
    //CQEdit::init();

    initialized = true;
  }
}

FileWidget::
FileWidget(Area *area, const QString &fileName) :
 Widget(area), fileName_(fileName)
{
  setObjectName("file");
}

void
FileWidget::
addWidgets()
{
  auto *layout = new QVBoxLayout(contents_);
  layout->setMargin(0); layout->setSpacing(0);

  //edit_ = new CQEdit;

  //edit_->getFile()->loadLines(fileName.toStdString());

  edit_ = new CQVi;

  edit_->loadFile(fileName_.toStdString());

  layout->addWidget(edit_);
}

QString
FileWidget::
id() const
{
  return QString("file.%1").arg(pos());
}

bool
FileWidget::
getNameValue(const QString &name, QVariant &value) const
{
  return Widget::getNameValue(name, value);
}

bool
FileWidget::
setNameValue(const QString &name, const QVariant &value)
{
  return Widget::setNameValue(name, value);
}

void
FileWidget::
draw(QPainter *, int /*dx*/, int /*dy*/)
{
}

QSize
FileWidget::
contentsSizeHint() const
{
  return QSize(-1, 400);
}

QSize
FileWidget::
contentsSize() const
{
  return QSize(width_, height_);
}

//------

void
FileTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file", ArgType::String, "tcl file");
}

QStringList
FileTclCmd::
getArgValues(const QString &option, const NameValueMap &nameValueMap)
{
  QStringList strs;

  if (option == "file") {
    auto p = nameValueMap.find("file");

    QString file = (p != nameValueMap.end() ? (*p).second : "");

    return Frame::s_completeFile(file);
  }

  return strs;
}

bool
FileTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  FileWidget *widget = nullptr;

  auto *area = frame_->larea();

  auto fileName = argv.getParseStr("file");

  widget = makeWidget<FileWidget>(area, fileName);

  return frame_->setCmdRc(widget->id());
}

//------

}

#endif
