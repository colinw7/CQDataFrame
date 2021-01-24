#ifndef CQDataFrameModel_H
#define CQDataFrameModel_H

#ifdef MODEL_DATA

#include <CQDataFrame.h>

class CQDataModel;
class CQModelDetails;

namespace CQDataFrame {

CQDATA_FRAME_TCL_CMD(Model)

CQDATA_FRAME_TCL_CMD(GetModelData)
CQDATA_FRAME_TCL_CMD(ShowModel)

#define ModelMgrInst ModelMgr::instance()

class ModelMgr {
 public:
  static ModelMgr *instance();

 public:
 ~ModelMgr() { }

  void addCmds(Frame *frame);

  QString addModel(CQDataModel *model);

  CQDataModel *getModel(const QString &id) const;

  CQModelDetails *getModelDetails(const QString &id) const;

 private:
  ModelMgr() { }

 private:
  struct ModelData {
    QString         id;
    CQDataModel*    model   { nullptr };
    CQModelDetails* details { nullptr };
  };

  using NamedModel = std::map<QString, ModelData>;

 private:
  NamedModel namedModel_;
};

}

#endif

#endif
