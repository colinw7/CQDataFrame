#ifndef CQDataFrameImage_H
#define CQDataFrameImage_H

#include <CQDataFrame.h>

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
  int width() const override;
  void setWidth (int w) override;

  //! get/set custom height
  int height() const override;
  void setHeight(int h) override;

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  void pasteText(const QString &) override;

  QSize calcSize() const override;

 private:
  void updateImage();

  void draw(QPainter *painter) override;

 private:
  QString file_;
  QImage  image_;
  QImage  simage_;
};

class ImageFactory : public WidgetFactory {
 public:
  ImageFactory() { }

  const char *name() const override { return "image"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("image", new ImageTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    auto *widget = new ImageWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif
