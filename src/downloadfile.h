#include <stdio.h>
#include <string>

namespace wenavi {
  namespace data {

    class DownloadFile
    {
    public:
      DownloadFile();
      DownloadFile(std::string filePath, size_t totalSize, bool initFullSize, bool writeTailFrequently);
      ~DownloadFile();

      void SetParams(std::string filePath, size_t totalSize, bool initFullSize, bool writeTailFrequently);

      bool Create();
      bool Close();

      size_t GetResumePosition();
      size_t Write(char* buffer, size_t size, size_t nmemb);

    private:
      FILE* m_fp;
      bool m_writeTailFrequently;
      bool m_initFullSize;
      std::string m_path;
      std::string m_tmpPath;
      size_t m_totalSize;
      size_t m_currentSize;
      size_t m_resumeSize;
      size_t m_writeCount;
    };
  }
}
