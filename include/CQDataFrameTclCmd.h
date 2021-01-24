#ifndef CQDataFrameTclCmd_H
#define CQDataFrameTclCmd_H

#include <CQTclCmd.h>

namespace CQDataFrame {

#define CQDATA_FRAME_TCL_CMD(NAME) \
class NAME##TclCmd : public CQTclCmd::CmdProc { \
 public: \
  enum class ArgType { \
    None    = int(CQTclCmd::CmdArg::Type::None), \
    Boolean = int(CQTclCmd::CmdArg::Type::Boolean), \
    Integer = int(CQTclCmd::CmdArg::Type::Integer), \
    Real    = int(CQTclCmd::CmdArg::Type::Real), \
    String  = int(CQTclCmd::CmdArg::Type::String), \
    SBool   = int(CQTclCmd::CmdArg::Type::SBool), \
    Enum    = int(CQTclCmd::CmdArg::Type::Enum) \
  }; \
\
  using CmdArg = CQTclCmd::CmdArg; \
\
 public: \
  NAME##TclCmd(Frame *frame) : \
    CmdProc(frame->tclCmdMgr()), frame_(frame) { } \
\
  bool exec(CQTclCmd::CmdArgs &args) override; \
\
  void addArgs(CQTclCmd::CmdArgs &args) override; \
\
  QStringList getArgValues(const QString &arg, \
                           const NameValueMap &nameValueMap=NameValueMap()) override; \
\
  CmdArg &addArg(CQTclCmd::CmdArgs &args, const QString &name, ArgType type, \
                 const QString &argDesc="", const QString &desc="") { \
    return args.addCmdArg(name, int(type), argDesc, desc); \
  } \
\
 public: \
  Frame *frame_ { nullptr }; \
};

#define CQDATA_FRAME_INST_TCL_CMD(NAME) \
class NAME##InstTclCmd : public CQTclCmd::CmdProc { \
 public: \
  enum class ArgType { \
    None    = int(CQTclCmd::CmdArg::Type::None), \
    Boolean = int(CQTclCmd::CmdArg::Type::Boolean), \
    Integer = int(CQTclCmd::CmdArg::Type::Integer), \
    Real    = int(CQTclCmd::CmdArg::Type::Real), \
    String  = int(CQTclCmd::CmdArg::Type::String), \
    SBool   = int(CQTclCmd::CmdArg::Type::SBool), \
    Enum    = int(CQTclCmd::CmdArg::Type::Enum) \
  }; \
\
  using CmdArg = CQTclCmd::CmdArg; \
\
 public: \
  NAME##InstTclCmd(Frame *frame, const QString &id) : \
   CmdProc(frame->tclCmdMgr()), frame_(frame), id_(id) { } \
\
  bool exec(CQTclCmd::CmdArgs &args) override; \
\
  void addArgs(CQTclCmd::CmdArgs &args) override; \
\
  QStringList getArgValues(const QString &name, \
                           const NameValueMap &nameValueMap=NameValueMap()) override; \
\
  CmdArg &addArg(CQTclCmd::CmdArgs &args, const QString &name, ArgType type, \
                 const QString &argDesc="", const QString &desc="") { \
    return args.addCmdArg(name, int(type), argDesc, desc); \
  } \
\
 public: \
  Frame*  frame_ { nullptr }; \
  QString id_; \
};

}

#endif
