#include <CQDataFrameModel.h>

#ifdef MODEL_DATA

#include <CQCsvModel.h>
#include <CQModelDetails.h>
#include <CQModelUtil.h>

namespace CQDataFrame {

ModelMgr *
ModelMgr::
instance()
{
  static ModelMgr *inst;

  if (! inst)
    inst = new ModelMgr;

  return inst;
}

void
ModelMgr::
addCmds(Frame *frame)
{
  frame->tclCmdMgr()->addCommand("model", new ModelTclCmd(frame));

  frame->tclCmdMgr()->addCommand("get_model_data", new GetModelDataTclCmd(frame));
  frame->tclCmdMgr()->addCommand("show_model"    , new ShowModelTclCmd   (frame));
}

QString
ModelMgr::
addModel(CQDataModel *model)
{
  auto id = QString("model.%1").arg(namedModel_.size() + 1);

  assert(namedModel_.find(id) == namedModel_.end());

  ModelData modelData;

  modelData.id      = id;
  modelData.model   = model;
  modelData.details = new CQModelDetails(modelData.model);

  namedModel_[id] = modelData;

  return id;
}

CQDataModel *
ModelMgr::
getModel(const QString &id) const
{
  auto p = namedModel_.find(id);
  if (p == namedModel_.end()) return nullptr;

  return (*p).second.model;
}

CQModelDetails *
ModelMgr::
getModelDetails(const QString &id) const
{
  auto p = namedModel_.find(id);
  if (p == namedModel_.end()) return nullptr;

  return (*p).second.details;
}

//------

void
ModelTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-file", ArgType::String, "model file");

  addArg(argv, "-comment_header"   , ArgType::Boolean,
                 "first comment line is header");
  addArg(argv, "-first_line_header", ArgType::Boolean,
                 "first line is header");
}

QStringList
ModelTclCmd::
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
ModelTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto fileName = argv.getParseStr("file");

  auto *model = new CQCsvModel();

  if (argv.hasParseArg("comment_header"))
    model->setCommentHeader(true);

  if (argv.hasParseArg("first_line_header"))
    model->setFirstLineHeader(true);

  if (! model->load(fileName)) {
    delete model;
    return false;
  }

  auto id = ModelMgrInst->addModel(model);

  return frame_->setCmdRc(id);
}

//------

void
GetModelDataTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-model" , ArgType::String, "model name");
  addArg(argv, "-name"  , ArgType::String, "data name");
  addArg(argv, "-column", ArgType::String, "column name");
}

QStringList
GetModelDataTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
GetModelDataTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto name = argv.getParseStr("name");

  if (name == "?") {
    auto strs = QStringList() << "num_columns" << "num_rows" << "num_unique";
    return frame_->setCmdRc(strs);
  }

  //---

  auto modelName = argv.getParseStr("model");

  auto *model = ModelMgrInst->getModel(modelName);
  if (! model) return false;

  if      (name == "num_columns")
    return frame_->setCmdRc(model->columnCount());
  else if (name == "num_rows")
    return frame_->setCmdRc(model->rowCount());

  auto *details = ModelMgrInst->getModelDetails(modelName);
  assert(details);

  int                   column        = -1;
  CQModelColumnDetails *columnDetails = nullptr;

  if (argv.hasParseArg("column")) {
    auto columnName = argv.getParseStr("column");

    column = details->lookupColumn(columnName);

    columnDetails = details->columnDetails(column);
  }

  if (name == "num_unique") {
    if (! columnDetails)
      return false;

    return frame_->setCmdRc(columnDetails->numUnique());
  }

  return true;
}

//------

void
ShowModelTclCmd::
addArgs(CQTclCmd::CmdArgs &argv)
{
  addArg(argv, "-model", ArgType::String, "model name");
}

QStringList
ShowModelTclCmd::
getArgValues(const QString &, const NameValueMap &)
{
  return QStringList();
}

bool
ShowModelTclCmd::
exec(CQTclCmd::CmdArgs &argv)
{
  addArgs(argv);

  bool rc;

  if (! argv.parse(rc))
    return rc;

  //---

  auto modelName = argv.getParseStr("model");

  auto *model = ModelMgrInst->getModel(modelName);
  if (! model) return false;

  //---

  std::cout << "<html>\n";
  std::cout << "<table width='100%' border>\n";

  int nr = std::min(model->rowCount(), 10);
  int nc = model->columnCount();

  for (int r = -1; r < nr; ++r) {
    std::cout << "<tr>\n";

    for (int c = 0; c < nc; ++c) {
      QString str;
      bool    ok;

      if (r < 0) {
        str = CQModelUtil::modelHeaderString(model, c, ok);

        std::cout << "<th>" << str.toStdString() << "</th>\n";
      }
      else {
        str = CQModelUtil::modelString(model, model->index(r, c, QModelIndex()), ok);

        std::cout << "<td>" << str.toStdString() << "</td>\n";
      }
    }

    std::cout << "</tr>\n";
  }

  std::cout << "</table>\n";

  return true;
}

//------

}

#endif
