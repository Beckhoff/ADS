// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsFile.h"
#include <iostream>
#include <list>

#pragma pack(push, 1)
/**
 * @brief This structure describes file information reveived via ADS
 *
 * Calling ReadWriteReqEx2 with IndexGroup == SYSTEMSERVICE_FFILEFIND
 * will return ADS file information in the provided readData buffer.
 * The data of that information is structured as TcFileFindData.
 */
struct TcFileFindData {
	uint32_t hFile;
	uint32_t dwFileAttributes;
	uint64_t nReserved1[5];
	char cFileName[260];
	char unused[14];
	uint16_t nReserved2;

	bool isDirectory(void) const
	{
		return 0x10 & dwFileAttributes;
	}

	void letoh(void)
	{
		hFile = bhf::ads::letoh(hFile);
		dwFileAttributes = bhf::ads::letoh(dwFileAttributes);
	}
};
#pragma pack(pop)

static bool FindNext(const AdsDevice &route, TcFileFindData &child,
		     const size_t length = 0, const char *const path = nullptr)
{
	const auto error = route.ReadWriteReqEx2(SYSTEMSERVICE_FFILEFIND,
						 child.hFile, sizeof(child),
						 &child, length, path, nullptr);
	// We reached the last child
	// If there is no more file in the current path the ADS service will
	// return ads error code 1804 so we can break and exit as expected.
	if (error && (error != 1804)) {
		throw AdsException(error);
	}
	// TwinCAT sends data in little endian so we have to convert it here
	child.letoh();
	return error == 1804;
}

static bool FindFirst(const AdsDevice &route, TcFileFindData &item,
		      const std::string &path)
{
	enum FFILEFIND : uint32_t {
		GENERIC = 1 << 0,
	};

	item.hFile = FFILEFIND::GENERIC;
	return FindNext(route, item, path.length(), path.c_str());
}

AdsFile::AdsFile(const AdsDevice &route, const std::string &filename,
		 const uint32_t flags)
	: m_Route(route)
	, m_Handle(route.OpenFile(filename, flags))
{
}

void AdsFile::Delete(const AdsDevice &route, const std::string &filename,
		     const uint32_t flags)
{
	auto error = route.ReadWriteReqEx2(SYSTEMSERVICE_FDELETE, flags, 0,
					   nullptr, filename.length(),
					   filename.c_str(), nullptr);

	if (error) {
		throw AdsException(error);
	}
}

int AdsFile::Find(const AdsDevice &route, const std::string &basePath,
		  const size_t maxdepth, std::ostream &os)
{
	struct Path {
		size_t depth;
		std::string path;
	};
	std::list<struct Path> pendingDirs{ { 0, basePath } };
	while (!pendingDirs.empty()) {
		auto path = pendingDirs.front().path;
		auto depth = pendingDirs.front().depth;
		pendingDirs.pop_front();

		TcFileFindData parent;
		if (FindFirst(route, parent, path)) {
			return 1804;
		}

		// Path exists print it and prepare traversing
		os << path << '\n';

		if (parent.isDirectory() && (depth < maxdepth)) {
			// Finding files in a directory is a bit weird. We get only one entry per call and for
			// every call we pass the last found item to get the next. The first item is special.
			// To get the children of a directory we have to append '/*'to the path of the directory.
			for (auto last = FindFirst(route, parent, path + "/*");
			     !last; last = FindNext(route, parent)) {
				if (parent.isDirectory()) {
					pendingDirs.push_back(
						{ depth + 1,
						  path + '/' +
							  parent.cFileName });
				} else {
					os << path << '/' << parent.cFileName
					   << '\n';
				}
			}
		}
	}
	return 0;
}

void AdsFile::Read(const size_t size, void *data, uint32_t &bytesRead) const
{
	auto error = m_Route.ReadWriteReqEx2(SYSTEMSERVICE_FREAD, *m_Handle,
					     size, data, 0, nullptr,
					     &bytesRead);

	if (error) {
		throw AdsException(error);
	}
}

void AdsFile::Write(const size_t size, const void *data) const
{
	auto error = m_Route.ReadWriteReqEx2(SYSTEMSERVICE_FWRITE, *m_Handle, 0,
					     nullptr, size, data, nullptr);
	if (error) {
		throw AdsException(error);
	}
}
