#ifndef CQDataFrameCanvas_H
#define CQDataFrameCanvas_H

#include <CQDataFrame.h>
#include <CDisplayRange2D.h>

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Canvas)

CQDATA_FRAME_INST_TCL_CMD(Canvas)

// canvas
class CanvasWidget : public Widget {
  Q_OBJECT

 public:
  CanvasWidget(Area *area, int width=100, int height=100);

  QString id() const override;

  bool getNameValue(const QString &name, QVariant &value) const override;
  bool setNameValue(const QString &name, const QVariant &value) override;

  //! get/set draw proc
  const QString &drawProc() const { return drawProc_; }
  void setDrawProc(const QString &s) { drawProc_ = s; }

  //! get/set xmin
  double xmin() const { return xmin_; }
  void setXMin(double r) { xmin_ = r; }

  //! get/set ymin
  double ymin() const { return ymin_; }
  void setYMin(double r) { ymin_ = r; }

  //! get/set xmax
  double xmax() const { return xmax_; }
  void setXMax(double r) { xmax_ = r; }

  //! get/set ymax
  double ymax() const { return ymax_; }
  void setYMax(double r) { ymax_ = r; }

  QSize calcSize() const override;

  void setBrush(const QBrush &brush);

  void setPen(const QPen &pen);

  void drawPath(const QString &path);

  void drawPixel(const QPointF &p);

  bool isMapping() const { return mapping_; }
  void setMapping(bool b) { mapping_ = b; }

  void windowToPixel(double wx, double wy, double *px, double *py) const;
  void pixelToWindow(double px, double py, double *wx, double *wy) const;

 private:
  void handleResize(int w, int h) override;

  void setWindowRange();

  void draw(QPainter *painter) override;

 private:
  using DisplayRange = CDisplayRange2D;

  QString      drawProc_;
  DisplayRange displayRange_;
  QImage       image_;
  QPainter*    ipainter_ { nullptr };
  bool         dirty_    { true };
  double       xmin_     { 0.0 };
  double       ymin_     { 0.0 };
  double       xmax_     { 100.0 };
  double       ymax_     { 100.0 };
  bool         mapping_  { true };
};

class CanvasFactory : public WidgetFactory {
 public:
  CanvasFactory() { }

  const char *name() const override { return "canvas"; }

  void addTclCommand(Frame *frame) override {
    frame->tclCmdMgr()->addCommand("canvas", new CanvasTclCmd(frame));
  }

  Widget *addWidget(Area *area) override {
    auto *widget = new CanvasWidget(area);

    area->addWidget(widget);

    return widget;
  }
};

}

#endif
