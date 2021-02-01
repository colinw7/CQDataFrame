#ifndef CQDataFrameImage_H
#define CQDataFrameImage_H

#include <CQDataFrame.h>
#include <CQDataFrameWidget.h>

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Image)

// image
class ImageWidget : public Widget {
  Q_OBJECT

  Q_PROPERTY(QString file READ file WRITE setFile  )

 public:
  ImageWidget(Area *area, const QString &file="");

  QString id() const override;

  //! get/set file name
  const QString &file() const { return file_; }
  void setFile(const QString &s);

  //! get/set custom width
  int contentsWidth() const override;
  void setContentsWidth(int w) override;

  //! get/set custom height
  int contentsHeight() const override;
  void setContentsHeight(int h) override;

  //! get/set name value
  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  void pasteText(const QString &) override;

  void addMenuItems(QMenu *menu) override;

  QSize contentsSizeHint() const override;
  QSize contentsSize() const override;

 private slots:
  void zoomInSlot();
  void zoomOutSlot();
  void resetSlot();

 private:
  void updateImage();

  void draw(QPainter *painter, int dx, int dy) override;

 private:
  QString file_;
  QImage  image_;
  QImage  simage_;
  double  xscale_ { 1.0 };
  double  yscale_ { 1.0 };
};

//---

class ImageFactory : public WidgetFactory {
 public:
  ImageFactory() { }

  const char *name() const override { return "image"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("image", new ImageTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    return makeWidget<ImageWidget>(area);
  }
};

}

#endif
