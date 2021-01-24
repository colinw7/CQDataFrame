#ifndef CQDataFrameFileText_H
#define CQDataFrameFileText_H

#include <QString>

namespace CQDataFrame {

struct FileText {
  enum class Type {
    FILENAME,
    TEXT
  };

  FileText() = default;

  FileText(Type type, const QString &value) :
   type(type), value(value) {
  }

  Type    type { Type::TEXT };
  QString value;
};

}

#endif
