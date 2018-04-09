#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string>

#include "downloadfile.h"

#define TMP_MAGIC 0xABCDE123
#define TMP_SUFFIX ".tmp"
#define HEADER_SIZE 12
#define WTAIL_BYTES 5242880	/* 5MB */
#define _CRT_SECURE_NO_WARNINGS

#ifdef WIN32
#include <windows.h>
static bool TruncateFile(const char* filePath, size_t size)
{
	WCHAR wFilePath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, filePath, strlen(filePath) + 1, wFilePath, sizeof(wFilePath));
	HANDLE file = CreateFile(wFilePath, GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == NULL) {
		return false;
	}
	SetFilePointer(file, size, NULL, FILE_BEGIN);
	SetEndOfFile(file);
	CloseHandle(file);
	return true;
}
#else
#include <unistd.h>
static bool TruncateFile(const char* filePath, size_t size)
{
	return (truncate(filePath, size) != -1);
}
#endif

DownloadFile::DownloadFile():
	m_fp(NULL), m_writeTailFrequently(false), m_initFullSize(false), m_totalSize(0), m_currentSize(0),
	m_resumeSize(0), m_writeCount(0) {}

DownloadFile::DownloadFile(std::string filePath, size_t totalSize, bool initFullSize, bool writeTailFrequently):
	m_fp(NULL), m_totalSize(0), m_currentSize(0), m_resumeSize(0), m_writeCount(0) 
{
	SetParams(filePath, totalSize, initFullSize, writeTailFrequently);
}

DownloadFile::~DownloadFile()
{
	if (m_fp != NULL) {
		Close();
	}
}

void DownloadFile::SetParams(std::string filePath, size_t totalSize, bool initFullSize, bool writeTailFrequently)
{
	m_path = filePath;
	m_tmpPath = filePath + TMP_SUFFIX;
	m_totalSize = totalSize;
	m_writeTailFrequently = writeTailFrequently;
	m_initFullSize = initFullSize;
}

bool DownloadFile::Create()
{
	FILE* fp = NULL;

	if (m_fp != NULL) {
		return false;
	}
#ifdef WIN32
	fopen_s(&fp, m_tmpPath.c_str(), "rb+");
#else
	fp = fopen(m_tmpPath.c_str(), "rb+");
#endif
	if (fp != NULL) {
		unsigned int headerData[3];
		fseek(fp, 0, SEEK_END);
		if (ftell(fp) <= HEADER_SIZE) {
			fclose(fp);
			remove(m_tmpPath.c_str());
			goto NEWFILE;
		}
		fseek(fp, -HEADER_SIZE, SEEK_END);
		size_t size = fread(headerData, 1, HEADER_SIZE, fp);
		if (size != HEADER_SIZE ||
				headerData[2] != TMP_MAGIC ||
				headerData[0] < headerData[1] ||
				headerData[0] != m_totalSize) {
			fclose(fp);
			remove(m_tmpPath.c_str());
			goto NEWFILE;
		}

		m_resumeSize = headerData[1];
		fseek(fp, headerData[1], SEEK_SET);

		printf("Resume at %ld\n", ftell(fp));

		m_fp = fp;
		return true;
	}

NEWFILE:
#ifdef WIN32
	fopen_s(&fp, m_tmpPath.c_str(), "w");
#else
	fp = fopen(m_tmpPath.c_str(), "w");
#endif
	if (fp == NULL)
		return false;
	fclose(fp);

	// resize file
	if (m_initFullSize) {
		if (!TruncateFile(m_tmpPath.c_str(), m_totalSize + HEADER_SIZE)) {
			return false;
		}
	}

#ifdef WIN32
	fopen_s(&fp, m_tmpPath.c_str(), "rb+");
#else
	fp = fopen(m_tmpPath.c_str(), "rb+");
#endif
	if (fp == NULL) {
		return false;
	}
	m_resumeSize = 0;
	m_fp = fp;

	return true;
}

bool DownloadFile::Close()
{
	unsigned int headerData[3];

	if (m_fp == NULL) {
		return false;
	}

	if (m_totalSize == m_currentSize + m_resumeSize) {
		fclose(m_fp);
		m_fp = NULL;
		TruncateFile(m_tmpPath.c_str(), m_totalSize);
		remove(m_path.c_str());
		rename(m_tmpPath.c_str(), m_path.c_str());
		printf("Download success length: %lu\n", m_totalSize);
		return true;
	}

	if (m_initFullSize) {
		fseek(m_fp, -HEADER_SIZE, SEEK_END);
	}
	headerData[0] = m_totalSize;
	headerData[1] = m_currentSize + m_resumeSize;
	headerData[2] = TMP_MAGIC;

	printf("Break at %lu\n", (size_t)headerData[1]);

	int n = fwrite(headerData, 1, HEADER_SIZE, m_fp);
	if (n != HEADER_SIZE) {
		fclose(m_fp);
		m_fp = NULL;
		printf("Write header failed.\n");
		return false;
	}
	fclose(m_fp);
	m_fp = NULL;
	printf("Write header ok %d [%d,%d,0x%X]\n", n, headerData[0], headerData[1], headerData[2]);
	return true;
}

size_t DownloadFile::GetResumePosition()
{
	if (m_fp == NULL) {
		return 0;
	}
	return m_resumeSize;
}

size_t DownloadFile::Write(char* buffer, size_t size, size_t nmemb)
{
	size_t wBytes;
	off_t curPos;
	unsigned int headerData[3];

	if (m_fp == NULL)
		return false;

	wBytes = fwrite(buffer, size, nmemb, m_fp);
	m_writeCount += wBytes;
	m_currentSize += wBytes;

	if (m_initFullSize && m_writeTailFrequently && m_writeCount >= WTAIL_BYTES) {
		headerData[0] = m_totalSize;
		headerData[1] = m_currentSize + m_resumeSize;
		headerData[2] = TMP_MAGIC;

		curPos = ftell(m_fp);
		fseek(m_fp, -HEADER_SIZE, SEEK_END);
		fwrite(headerData, 1, HEADER_SIZE, m_fp);
		fseek(m_fp, curPos, SEEK_SET);
		m_writeCount = 0;
	}

	return wBytes;
}
