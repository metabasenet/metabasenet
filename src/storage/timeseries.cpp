// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "timeseries.h"

using namespace std;
using namespace boost::filesystem;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CTimeSeriesBase

CTimeSeriesBase::CTimeSeriesBase()
{
    nLastFile = 0;
}

CTimeSeriesBase::~CTimeSeriesBase()
{
}

bool CTimeSeriesBase::Initialize(const path& pathLocationIn, const string& strPrefixIn)
{
    if (!exists(pathLocationIn))
    {
        create_directories(pathLocationIn);
    }

    if (!is_directory(pathLocationIn))
    {
        return false;
    }

    pathLocation = pathLocationIn;
    strPrefix = strPrefixIn;
    nLastFile = 1;

    for (;;)
    {
        path last = pathLocation / FileName(nLastFile + 1);
        if (!exists(last))
        {
            break;
        }
        nLastFile++;
    }
    return CheckDiskSpace();
}

void CTimeSeriesBase::Deinitialize()
{
}

bool CTimeSeriesBase::CheckDiskSpace()
{
    // 15M
    return (space(pathLocation).available > 15000000);
}

const std::string CTimeSeriesBase::FileName(uint32 nFile)
{
    ostringstream oss;
    oss << strPrefix << "_" << setfill('0') << setw(6) << nFile << ".dat";
    return oss.str();
}

bool CTimeSeriesBase::GetFilePath(uint32 nFile, string& strPath)
{
    path current = pathLocation / FileName(nFile);
    if (exists(current) && is_regular_file(current))
    {
        strPath = current.string();
        return true;
    }
    return false;
}

bool CTimeSeriesBase::GetLastFilePath(uint32& nFile, std::string& strPath, const uint32 nWriteDataSize)
{
    for (;;)
    {
        path last = pathLocation / FileName(nLastFile);
        if (!exists(last))
        {
            FILE* fp = fopen(last.string().c_str(), "w+");
            if (fp == nullptr)
            {
                break;
            }
            fclose(fp);
        }
        if (is_regular_file(last) && file_size(last) + nWriteDataSize + 8 <= MAX_FILE_SIZE)
        {
            nFile = nLastFile;
            strPath = last.string();
            return true;
        }
        nLastFile++;
    }
    return false;
}

bool CTimeSeriesBase::RemoveFollowUpFile(uint32 nBeginFile)
{
    std::string pathFile;
    while (GetFilePath(nBeginFile, pathFile))
    {
        if (!boost::filesystem::remove(path(pathFile)))
        {
            mtbase::StdError("TimeSeriesBase", "RemoveFollowUpFile: remove fail fail, file: %s", pathFile.c_str());
            return false;
        }
        ++nBeginFile;
    }
    return true;
}

bool CTimeSeriesBase::TruncateFile(const string& pathFile, uint32 nOffset)
{
    string strTempFilePath = pathFile + ".temp";
    boost::filesystem::rename(path(pathFile), path(strTempFilePath));

    FILE* pReadFd = nullptr;
    FILE* pWriteFd = nullptr;
    pReadFd = fopen(strTempFilePath.c_str(), "rb");
    if (pReadFd == nullptr)
    {
        mtbase::StdError("TimeSeriesBase", "TruncateFile: fopen fail1, file: %s", strTempFilePath.c_str());
        return false;
    }
    pWriteFd = fopen(pathFile.c_str(), "wb");
    if (pWriteFd == nullptr)
    {
        mtbase::StdError("TimeSeriesBase", "TruncateFile: fopen fail2, file: %s", pathFile.c_str());
        fclose(pReadFd);
        return false;
    }
    uint32 nReadLen = 0;
    uint8 uReadBuf[4096] = { 0 };
    while (nReadLen < nOffset && !feof(pReadFd))
    {
        size_t nNeedReadLen = nOffset - nReadLen;
        if (nNeedReadLen > sizeof(uReadBuf))
        {
            nNeedReadLen = sizeof(uReadBuf);
        }
        size_t nLen = fread(uReadBuf, 1, nNeedReadLen, pReadFd);
        if (nLen > 0)
        {
            fwrite(uReadBuf, 1, nLen, pWriteFd);
            nReadLen += nLen;
        }
        if (nLen != nNeedReadLen && ferror(pReadFd))
        {
            break;
        }
    }
    fclose(pReadFd);
    fclose(pWriteFd);

    boost::filesystem::remove(path(strTempFilePath));
    return true;
}

bool CTimeSeriesBase::RepairFile(uint32 nFile, uint32 nOffset)
{
    if (nOffset > 0)
    {
        std::string pathFile;
        if (!GetFilePath(nFile, pathFile))
        {
            mtbase::StdError("TimeSeriesBase", "RepairFile: GetFilePath fail");
            return false;
        }
        if (!TruncateFile(pathFile, nOffset))
        {
            mtbase::StdError("TimeSeriesBase", "RepairFile: TruncateFile fail");
            return false;
        }
        return RemoveFollowUpFile(nFile + 1);
    }
    else
    {
        return RemoveFollowUpFile(nFile);
    }
}

//////////////////////////////
// CTimeSeriesCached

const uint32 CTimeSeriesCached::nMagicNum = 0x8A5CA1E8;

CTimeSeriesCached::CTimeSeriesCached()
  : cacheStream(FILE_CACHE_SIZE)
{
}

CTimeSeriesCached::~CTimeSeriesCached()
{
}

bool CTimeSeriesCached::Initialize(const path& pathLocationIn, const string& strPrefixIn)
{

    if (!CTimeSeriesBase::Initialize(pathLocationIn, strPrefixIn))
    {
        return false;
    }

    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        ResetCache();
    }
    return true;
}

void CTimeSeriesCached::Deinitialize()
{
    boost::unique_lock<boost::mutex> lock(mtxCache);

    ResetCache();
}

void CTimeSeriesCached::ResetCache()
{
    cacheStream.Clear();
    mapCachePos.clear();
}

bool CTimeSeriesCached::VacateCache(uint32 nNeeded)
{
    const size_t nHdrSize = 12;

    while (cacheStream.GetBufFreeSpace() < nNeeded + nHdrSize)
    {
        CDiskPos diskpos;
        uint32 nSize = 0;

        try
        {
            cacheStream.Rewind();
            cacheStream >> diskpos >> nSize;
        }
        catch (exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }

        mapCachePos.erase(diskpos);
        cacheStream.Consume(nSize + nHdrSize);
    }
    return true;
}

//////////////////////////////
// CTimeSeriesChunk

const uint32 CTimeSeriesChunk::nMagicNum = 0x8A5CA1E8;

CTimeSeriesChunk::CTimeSeriesChunk()
{
}

CTimeSeriesChunk::~CTimeSeriesChunk()
{
}

} // namespace storage
} // namespace metabasenet
