#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>

#ifdef WIN32
#include <curl.h>
#else
#include <curl/curl.h>
#endif

#include "downloadfile.h"

#define SKIP_VAR(a) (a = a);

CURL* g_curl;
DownloadFile* g_downloadFile;
bool g_stopFlag;

static size_t GetRemoteFileLength(const char* url)
{
	double downloadFileLength = 0;
	CURL* pCurl = curl_easy_init();
	curl_easy_setopt(pCurl, CURLOPT_URL, url);
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 5000);
	curl_easy_setopt(pCurl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(pCurl, CURLOPT_NOBODY, 1L);

	if(curl_easy_perform(pCurl) == CURLE_OK) {
		curl_easy_getinfo(pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLength);
	}

	curl_easy_cleanup(pCurl);
	return (size_t)downloadFileLength;
}

static size_t WriteFunc(char *str, size_t size, size_t nmemb, void *data)
{
	SKIP_VAR(data);
	size_t wSize = g_downloadFile->Write(str, size, nmemb);
	return wSize;
}

static int ProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	SKIP_VAR(clientp);
	SKIP_VAR(dltotal);
	SKIP_VAR(dlnow);
	SKIP_VAR(ultotal);
	SKIP_VAR(ulnow);
	if (g_stopFlag)
		return CURLE_ABORTED_BY_CALLBACK;
	return CURLE_OK;
}

static bool StartDownload(const char* url)
{
	CURL* pCurl = curl_easy_init();
	g_curl = pCurl;
	CURLcode result;
	size_t resumeSize;

	resumeSize = g_downloadFile->GetResumePosition();

	curl_easy_setopt(pCurl, CURLOPT_URL, url);
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 5000);
	curl_easy_setopt(pCurl, CURLOPT_HEADER, 0L);
	curl_easy_setopt(pCurl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(pCurl, CURLOPT_RESUME_FROM, resumeSize);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteFunc);
	curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(pCurl, CURLOPT_PROGRESSFUNCTION, ProgressFunc);

	result = curl_easy_perform(pCurl);

	if (g_curl != NULL) {
		curl_easy_cleanup(g_curl);
		g_curl = NULL;
	}

	return result == CURLE_OK;
}

static void CatchSignal(int)
{
	g_stopFlag = true;
}

int main(int argc, char** argv)
{
	curl_global_init(CURL_GLOBAL_ALL);
	size_t totalSize;
	bool result;

	signal(SIGINT, CatchSignal);

	if (argc != 3) {
		printf("[Usage] %s <URL> <SaveFile>\n", argv[0]);
		goto END;
	}

	totalSize = GetRemoteFileLength(argv[1]);
	if (totalSize == 0) {
		printf("Can't get URL.\n");
		goto END;
	}

	g_downloadFile = new DownloadFile(argv[2], totalSize, false, false);

	result = g_downloadFile->Create();
	if (!result) {
		printf("Create local file failed.\n");
		goto END;
	}

	StartDownload(argv[1]);

END:
	if (g_downloadFile) {
		g_downloadFile->Close();
		delete g_downloadFile;
		g_downloadFile = NULL;
	}
	curl_global_cleanup();
	return 0;
}
