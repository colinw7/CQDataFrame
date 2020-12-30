#ifndef CFileMatch_H
#define CFileMatch_H

#include <string>
#include <vector>
#include <list>

class CGlob;

class CFileMatch {
 public:
  CFileMatch();
 ~CFileMatch();

  void addIgnorePattern(const std::string &pattern);

  void setOnlyExec(bool exec=true) { only_exec_ = exec; }

  std::string mostMatchPrefix(const std::string &prefix);
  std::string mostMatchPattern(const std::string &pattern);

  bool matchPrefix(const std::string &prefix, std::vector<std::string> &files);
  bool matchPattern(const std::string &pattern, std::vector<std::string> &files);

  bool matchCurrentDir(const std::string &pattern, std::vector<std::string> &files);

 private:
  bool isIgnoreFile(const std::string &fileName);

 private:
  typedef std::list<CGlob *> GlobList;

  GlobList ignore_patterns_;
  bool     only_exec_;
};

#endif
