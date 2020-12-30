#include <CFileMatch.h>

#include <CDir.h>
#include <CStrUtil.h>
#include <CGlob.h>

CFileMatch::
CFileMatch() :
 only_exec_(false)
{
}

CFileMatch::
~CFileMatch()
{
  GlobList::iterator pglob1 = ignore_patterns_.begin();
  GlobList::iterator pglob2 = ignore_patterns_.end();

  for ( ; pglob1 != pglob2; ++pglob1)
    delete *pglob1;
}

void
CFileMatch::
addIgnorePattern(const std::string &pattern)
{
  CGlob *glob = new CGlob(pattern);

  ignore_patterns_.push_back(glob);
}

std::string
CFileMatch::
mostMatchPrefix(const std::string &prefix)
{
  std::vector<std::string> files;

  if (! CFileMatch::matchPrefix(prefix, files))
    return prefix;

  return CStrUtil::mostMatch(files);
}

std::string
CFileMatch::
mostMatchPattern(const std::string &pattern)
{
  std::vector<std::string> files;

  if (! CFileMatch::matchPattern(pattern, files))
    return pattern;

  return CStrUtil::mostMatch(files);
}

bool
CFileMatch::
matchPrefix(const std::string &prefix, std::vector<std::string> &files)
{
  std::string pattern = prefix + "*";

  if (! matchPattern(pattern, files))
    return false;

  return true;
}

bool
CFileMatch::
matchPattern(const std::string &pattern, std::vector<std::string> &files)
{
  CStrWords words = CStrUtil::toFields(pattern, "/");

  int num_words = words.size();

  if (num_words == 0)
    return false;

  std::vector<std::string> words1;

  int i = 0;

  for ( ; i < num_words; i++) {
    std::string word1 = words[i].getWord();;
    std::string word2;

    if (CFile::expandTilde(word1, word2))
      words1.push_back(word2);
    else
      words1.push_back(word1);
  }

  i = 0;

  while (i < num_words && words1[i].size() == 0)
    i++;

  if (i >= num_words)
    return false;

  if (pattern.size() > 0 && pattern[0] == '/')
    files.push_back("/");
  else
    files.push_back("");

  for ( ; i < num_words; i++) {
    if (words1[i].size() == 0)
      continue;

    std::vector< std::vector<std::string> > files2_array;

    int num_files = files.size();

    for (int j = 0; j < num_files; j++) {
      if (files[j] != "") {
        if (! CFile::exists(files[j]) || ! CFile::isDirectory(files[j]))
          continue;

        CDir::enter(files[j]);
      }

      std::vector<std::string> files1;

      matchCurrentDir(words1[i], files1);

      if (files1.size() > 0) {
        std::vector<std::string> files2;

        int num_files1 = files1.size();

        for (int k = 0; k < num_files1; k++) {
          if (words1[i][0] != '.' &&
              (files1[k].size() > 0 && files1[k][0] == '.'))
            continue;

          std::string file = files[j];

          if (file.size() > 0 && file[file.size() - 1] != '/')
            file += "/";

          file += files1[k];

          files2.push_back(file);
        }

        files2_array.push_back(files2);
      }

      if (files[j] != "")
        CDir::leave();
    }

    files.clear();

    int num_files2_array = files2_array.size();

    for (int j = 0; j < num_files2_array; j++)
      copy(files2_array[j].begin(), files2_array[j].end(),
           back_inserter(files));
  }

  int num_files = files.size();

  std::string word;

  for (i = 0; i < num_files; i++) {
    if (CFile::addTilde(files[i], word))
      files[i] = word;
  }

  return true;
}

bool
CFileMatch::
matchCurrentDir(const std::string &pattern, std::vector<std::string> &files)
{
  if (pattern == "." || pattern == "..") {
    files.push_back(pattern);

    return true;
  }

  if (CFile::exists(pattern)) {
    files.push_back(pattern);

    return true;
  }

  CDir dir(".");

  CGlob glob(pattern);

  glob.setAllowOr(false);
  glob.setAllowNonPrintable(true);

  std::vector<std::string> filenames;

  (void) dir.getFilenames(filenames);

  int num_filenames = filenames.size();

  for (int i = 0; i < num_filenames; i++) {
    const std::string &fileName = filenames[i];

    if (isIgnoreFile(fileName))
      continue;

    if (only_exec_ && ! CFile::isExecutable(fileName))
      continue;

    if (glob.compare(fileName))
      files.push_back(fileName);
  }

  return true;
}

bool
CFileMatch::
isIgnoreFile(const std::string &fileName)
{
  GlobList::iterator pglob1 = ignore_patterns_.begin();
  GlobList::iterator pglob2 = ignore_patterns_.end();

  for ( ; pglob1 != pglob2; ++pglob1)
    if ((*pglob1)->compare(fileName))
      return true;

  return false;
}
