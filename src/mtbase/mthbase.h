// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __MTHBASE_H
#define __MTHBASE_H

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <queue>

#include "type.h"
#include "util.h"

namespace mtbase
{

inline unsigned long GetCurThreadId()
{
    std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long threadNumber = 0;
    threadNumber = std::stoul(threadId, nullptr, 16);
    return threadNumber;
}

class CBaseUniqueId : public boost::noncopyable
{
public:
    CBaseUniqueId(const uint64 ui64Id)
      : ui64UniqueId(ui64Id) {}
    CBaseUniqueId(CBaseUniqueId& uid)
      : ui64UniqueId(uid.ui64UniqueId) {}

    uint8 GetPortType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucPortType;
    }
    uint8 GetDirection() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucDirection;
    }
    uint8 GetType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucType;
    }
    uint8 GetSubType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucSubType;
    }
    uint32 GetId() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->uiId;
    }
    uint64 GetUniqueId() const
    {
        return ui64UniqueId;
    }

    CBaseUniqueId& operator=(const CBaseUniqueId& uid)
    {
        ui64UniqueId = uid.ui64UniqueId;
        return *this;
    }

    static uint64 CreateUniqueId(const uint8 ucPortType, const uint8 ucDirection, const uint8 ucType, const uint8 ucSubType = 0);

private:
    static uint32 uiIdCreate;
    static boost::mutex lockCreate;

#pragma pack(1)
    typedef struct _BASE_UNIQUE_ID
    {
        uint32 uiId;
        uint8 ucPortType : 1;  //0: tcp, 1: udp
        uint8 ucDirection : 1; //0: in, 1: out
        uint8 ucReserve : 6;
        uint8 ucType;
        uint8 ucSubType;
        uint8 ucRand;
    } BASE_UNIQUE_ID, *PBASE_UNIQUE_ID;
#pragma pack()

    uint64 ui64UniqueId;
};

class CMthWait;

class CMthEvent
{
public:
    CMthEvent(const bool bManualResetIn = false, const bool bInitSigStateIn = false)
      : fManualReset(bManualResetIn), fSingleFlag(bInitSigStateIn)
    {
        ui64EventId = CBaseUniqueId::CreateUniqueId(0, 0, 0xEF);
    }
    ~CMthEvent()
    {
        mapWait.clear();
    }

    uint64 GetEventId() const
    {
        return ui64EventId;
    }

    void SetEvent();
    void ResetEvent();

    bool AddWait(CMthWait* pWait);
    void DelWait(CMthWait* pWait);

    bool Wait(const uint32 ui32Timeout); /* ui32Timeout is milliseconds */

    bool QuerySetWaitObj(void* pWaitObj);
    void ClearWaitObj(void* pWaitObj);
    void ClearAllWaitObj();

private:
    boost::mutex lockEvent;
    boost::condition_variable_any condEvent;

    bool fSingleFlag;
    bool fManualReset;

    uint64 ui64EventId;

    std::map<uint64, CMthWait*> mapWait;
};

class CMthWait
{
public:
    CMthWait()
      : ui32WaitPos(0)
    {
        ui64WaitId = CBaseUniqueId::CreateUniqueId(0, 0, 0xEF);
    }
    ~CMthWait();

    uint64 GetWaitId() const
    {
        return ui64WaitId;
    }
    bool AddEvent(CMthEvent* pEvent, const int iEventFlag);
    void DelEvent(CMthEvent* pEvent);
    void SetSignal(CMthEvent* pEvent);
    int Wait(const uint32 ui32Timeout); /* ui32Timeout is milliseconds */

private:
    boost::mutex lockWait;
    boost::condition_variable_any condWait;
    uint64 ui64WaitId;

    typedef struct _NM_EVENT
    {
        int iEventFlag;
        bool fSignalFlag;
        CMthEvent* pEvent;
    } NM_EVENT, *PNM_EVENT;

    std::map<uint64, PNM_EVENT> mapEvent;
    uint32 ui32WaitPos;
};

class CMthDataBuf
{
public:
    CMthDataBuf()
      : pDataBuf(NULL), ui32BufSize(0), ui32DataLen(0) {}
    CMthDataBuf(const uint32 ui32AllocSize)
    {
        ui32BufSize = ui32AllocSize;
        ui32DataLen = 0;
        pDataBuf = new char[ui32BufSize];
    }
    CMthDataBuf(const char* pInBuf, const uint32 ui32InLen)
    {
        pDataBuf = NULL;
        ui32BufSize = 0;
        ui32DataLen = 0;

        assign(pInBuf, ui32InLen);
    }
    CMthDataBuf(const std::string& strIn)
    {
        pDataBuf = NULL;
        ui32BufSize = 0;
        ui32DataLen = 0;

        assign(strIn.data(), strIn.size());
    }
    CMthDataBuf(const bytes& btData)
    {
        pDataBuf = NULL;
        ui32BufSize = 0;
        ui32DataLen = 0;

        assign((const char*)(btData.data()), btData.size());
    }
    CMthDataBuf(const CMthDataBuf& mbuf)
    {
        pDataBuf = NULL;
        ui32BufSize = 0;
        ui32DataLen = 0;

        assign(mbuf.GetDataBuf(), mbuf.GetDataLen());
    }
    ~CMthDataBuf()
    {
        clear();
    }

    inline char* GetDataBuf() const
    {
        return pDataBuf;
    }
    inline uint32 GetBufSize() const
    {
        return ui32BufSize;
    }
    inline uint32 GetDataLen() const
    {
        return ui32DataLen;
    }
    void GetData(bytes& btData)
    {
        if (pDataBuf && ui32DataLen > 0)
        {
            btData.assign(pDataBuf, pDataBuf + ui32DataLen);
        }
    }

    CMthDataBuf& operator=(const CMthDataBuf& mbuf)
    {
        if (mbuf.GetDataBuf() && mbuf.GetDataLen() > 0)
        {
            assign(mbuf.GetDataBuf(), mbuf.GetDataLen());
        }
        return *this;
    }
    CMthDataBuf& operator+=(const CMthDataBuf& mbuf)
    {
        add(mbuf.GetDataBuf(), mbuf.GetDataLen());
        return *this;
    }

    void assign(const char* pInBuf, const uint32 ui32InLen)
    {
        if (pDataBuf)
        {
            delete[] pDataBuf;
            pDataBuf = NULL;
        }
        ui32BufSize = 0;
        ui32DataLen = 0;

        if (pInBuf && ui32InLen > 0)
        {
            ui32BufSize = ui32InLen;
            ui32DataLen = ui32BufSize;
            pDataBuf = new char[ui32BufSize];
            memcpy(pDataBuf, pInBuf, ui32InLen);
        }
    }
    void add(const char* pData, const uint32 nLen)
    {
        if (pData && nLen > 0)
        {
            reserve(ui32DataLen + nLen);
            memcpy(pDataBuf + ui32DataLen, pData, nLen);
            ui32DataLen += nLen;
        }
    }
    void insert(const uint32 pos, const char* pData, const uint32 nLen)
    {
        if (pData && nLen > 0)
        {
            uint32 posInsert = pos;
            if (pDataBuf == nullptr)
            {
                posInsert = 0;
            }
            else if (posInsert > ui32DataLen)
            {
                posInsert = ui32DataLen;
            }
            ui32BufSize = ui32DataLen + nLen;
            char* pNewBuf = new char[ui32BufSize];
            if (pDataBuf && posInsert > 0)
            {
                memcpy(pNewBuf, pDataBuf, posInsert);
            }
            memcpy(pNewBuf + posInsert, pData, nLen);
            if (pDataBuf && ui32DataLen > posInsert)
            {
                memcpy(pNewBuf + posInsert + nLen, pDataBuf + posInsert, ui32DataLen - posInsert);
            }
            if (pDataBuf)
            {
                delete[] pDataBuf;
                pDataBuf = NULL;
            }
            pDataBuf = pNewBuf;
            ui32DataLen += nLen;
        }
    }
    void erase(const uint32 ui32Pos, const uint32 ui32Len)
    {
        if (pDataBuf && ui32DataLen > 0 && ui32Len > 0 && ui32Pos < ui32DataLen)
        {
            if (ui32Pos + ui32Len >= ui32DataLen)
            {
                ui32DataLen = ui32Pos;
            }
            else
            {
                memmove(pDataBuf + ui32Pos,
                        pDataBuf + ui32Pos + ui32Len,
                        ui32DataLen - (ui32Pos + ui32Len));
                ui32DataLen -= ui32Len;
            }
        }
    }
    void erase(const uint32 ui32Len)
    {
        erase(0, ui32Len);
    }
    void reserve(const uint32 ui32Size)
    {
        if (ui32Size > ui32BufSize || pDataBuf == NULL)
        {
            ui32BufSize = ui32Size;
            char* pNewBuf = new char[ui32BufSize];
            if (pDataBuf && ui32DataLen > 0)
            {
                memcpy(pNewBuf, pDataBuf, ui32DataLen);
            }
            if (pDataBuf)
            {
                delete[] pDataBuf;
            }
            pDataBuf = pNewBuf;
        }
    }
    void clear()
    {
        if (pDataBuf)
        {
            delete[] pDataBuf;
            pDataBuf = NULL;
        }
        ui32BufSize = 0;
        ui32DataLen = 0;
    }

protected:
    char* pDataBuf;
    uint32 ui32BufSize;
    uint32 ui32DataLen;
};

template <typename T>
class CMthNvDataBuf : public CMthDataBuf
{
public:
    CMthNvDataBuf() {}
    CMthNvDataBuf(T v, const char* p, const uint32 n)
      : CMthDataBuf(p, n), tNv(v) {}
    CMthNvDataBuf(T v, const std::string s)
      : CMthDataBuf(s), tNv(v) {}

    T& SetNvData(T& v)
    {
        tNv = v;
    }
    T& GetNvData()
    {
        return tNv;
    }

private:
    T tNv;
};

template <typename T>
class CMthQueue
{
public:
    CMthQueue()
      : fAbort(false), eventRead(true, false), eventWrite(true, true), ui32MaxQueueSize(10000) {}
    CMthQueue(const uint32 ui32QueueSize)
      : fAbort(false), eventRead(true, false), eventWrite(true, true), ui32MaxQueueSize(ui32QueueSize) {}
    ~CMthQueue() {}

    /* ui32Timeout is milliseconds */
    virtual bool SetData(T& data, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return PrtSetData(lock, data, ui32Timeout);
    }

    virtual bool SetData(const T& data, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return PrtSetData(lock, data, ui32Timeout);
    }

    /* ui32Timeout is milliseconds */
    virtual bool SetMoreData(const std::vector<T>& vDataList, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return PrtSetMoreData(lock, vDataList, ui32Timeout);
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetData(T& data, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return PrtGetData(lock, data, ui32Timeout);
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetMoreData(std::vector<T>& vDataList, const uint32 nMaxGetCount = 0, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return PrtGetMoreData(lock, vDataList, nMaxGetCount, ui32Timeout);
    }

    uint32 GetCount()
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        return qData.size();
    }

    CMthEvent& GetReadEvent()
    {
        return eventRead;
    }
    CMthEvent& GetWriteEvent()
    {
        return eventWrite;
    }
    void Interrupt()
    {
        boost::unique_lock<boost::mutex> lock(lockQueue);
        fAbort = true;
        condQueueRead.notify_all();
        condQueueWrite.notify_all();
    }

    void SetQueueSize(const uint32 ui32QueueSize)
    {
        if (ui32QueueSize > 0)
        {
            boost::unique_lock<boost::mutex> lock(lockQueue);
            ui32MaxQueueSize = ui32QueueSize;
        }
    }

protected:
    inline bool PrtSetData(boost::unique_lock<boost::mutex>& lock, const T& data, const uint32 ui32Timeout)
    {
        if (fAbort)
        {
            return false;
        }
        uint64 ui64BeginTime = GetTimeMillis();
        do
        {
            if (qData.size() < ui32MaxQueueSize)
            {
                qData.push(data);
                condQueueRead.notify_one();
                eventRead.SetEvent();
                if (qData.size() >= ui32MaxQueueSize)
                {
                    eventWrite.ResetEvent();
                }
                return true;
            }
            if (ui32Timeout)
            {
                uint64 ui64WaitTime = ui32Timeout - (GetTimeMillis() - ui64BeginTime);
                if (ui64WaitTime <= 0 || !condQueueWrite.timed_wait(lock, boost::posix_time::milliseconds(ui64WaitTime)))
                {
                    break;
                }
            }
        } while (!fAbort && ui32Timeout);

        return false;
    }

    inline bool PrtSetMoreData(boost::unique_lock<boost::mutex>& lock, const std::vector<T>& vDataList, const uint32 ui32Timeout)
    {
        if (fAbort)
        {
            return false;
        }
        uint64 ui64BeginTime = GetTimeMillis();
        do
        {
            if (qData.size() < ui32MaxQueueSize)
            {
                for (auto& vd : vDataList)
                {
                    qData.push(vd);
                }
                if (vDataList.size() > 1)
                {
                    condQueueRead.notify_all();
                }
                else
                {
                    condQueueRead.notify_one();
                }
                eventRead.SetEvent();
                if (qData.size() >= ui32MaxQueueSize)
                {
                    eventWrite.ResetEvent();
                }
                return true;
            }
            if (ui32Timeout)
            {
                uint64 ui64WaitTime = ui32Timeout - (GetTimeMillis() - ui64BeginTime);
                if (ui64WaitTime <= 0 || !condQueueWrite.timed_wait(lock, boost::posix_time::milliseconds(ui64WaitTime)))
                {
                    break;
                }
            }
        } while (!fAbort && ui32Timeout);

        return false;
    }

    inline bool PrtGetData(boost::unique_lock<boost::mutex>& lock, T& data, const uint32 ui32Timeout)
    {
        if (fAbort)
        {
            return false;
        }
        uint64 ui64BeginTime = GetTimeMillis();
        do
        {
            if (!qData.empty())
            {
                data = qData.front();
                qData.pop();
                condQueueWrite.notify_one();
                eventWrite.SetEvent();
                if (qData.empty())
                {
                    eventRead.ResetEvent();
                }
                return true;
            }
            if (ui32Timeout)
            {
                uint64 ui64WaitTime = ui32Timeout - (GetTimeMillis() - ui64BeginTime);
                if (ui64WaitTime <= 0 || !condQueueRead.timed_wait(lock, boost::posix_time::milliseconds(ui64WaitTime)))
                {
                    break;
                }
            }
        } while (!fAbort && ui32Timeout);

        return false;
    }

    inline bool PrtGetMoreData(boost::unique_lock<boost::mutex>& lock, std::vector<T>& vDataList, const uint32 nMaxGetCount, const uint32 ui32Timeout)
    {
        if (fAbort)
        {
            return false;
        }
        uint64 ui64BeginTime = GetTimeMillis();
        do
        {
            if (!qData.empty())
            {
                while (!qData.empty())
                {
                    vDataList.push_back(qData.front());
                    qData.pop();
                    if (nMaxGetCount != 0 && vDataList.size() >= nMaxGetCount)
                    {
                        break;
                    }
                }
                if (vDataList.size() > 1)
                {
                    condQueueWrite.notify_all();
                }
                else
                {
                    condQueueWrite.notify_one();
                }
                eventWrite.SetEvent();
                if (qData.empty())
                {
                    eventRead.ResetEvent();
                }
                return true;
            }
            if (ui32Timeout)
            {
                uint64 ui64WaitTime = ui32Timeout - (GetTimeMillis() - ui64BeginTime);
                if (ui64WaitTime <= 0 || !condQueueRead.timed_wait(lock, boost::posix_time::milliseconds(ui64WaitTime)))
                {
                    break;
                }
            }
        } while (!fAbort && ui32Timeout);

        return false;
    }

protected:
    boost::mutex lockQueue;
    boost::condition_variable_any condQueueRead;
    boost::condition_variable_any condQueueWrite;

    bool fAbort;
    CMthEvent eventRead;
    CMthEvent eventWrite;

    std::queue<T> qData;
    uint32 ui32MaxQueueSize;
};

template <typename T>
class CTaskQueue : public CMthQueue<T>
{
public:
    CTaskQueue()
      : fTasking(false), CMthQueue<T>() {}
    CTaskQueue(const uint32 ui32QueueSize)
      : fTasking(false), CMthQueue<T>(ui32QueueSize) {}
    ~CTaskQueue() {}

    /* ui32Timeout is milliseconds */
    virtual bool SetData(T& data, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetData(lock, data, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    virtual bool SetData(const T& data, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetData(lock, data, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool SetMoreData(const std::vector<T>& vDataList, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetMoreData(lock, vDataList, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetData(T& data, const uint32 ui32Timeout = 0) override
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtGetData(lock, data, ui32Timeout))
        {
            CancelTask();
            return false;
        }
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetMoreData(std::vector<T>& vDataList, const uint32 nMaxGetCount = 0, const uint32 ui32Timeout = 0) override
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtGetMoreData(lock, vDataList, nMaxGetCount, ui32Timeout))
        {
            CancelTask();
            return false;
        }
        return true;
    }

protected:
    virtual bool CheckTask()
    {
        if (!fTasking)
        {
            fTasking = true;
            return true;
        }
        return false;
    }
    virtual void CancelTask()
    {
        fTasking = false;
    }

protected:
    bool fTasking;
};

template <typename T>
class CMoreTaskQueue : public CMthQueue<T>
{
public:
    CMoreTaskQueue()
      : nRunTaskCount(0), nMaxTaskCount(1), nIncCount(1000), CMthQueue<T>() {}
    CMoreTaskQueue(const uint32 nMaxTaskCountIn, const uint64 nIncCountIn, const uint32 ui32QueueSize)
      : nRunTaskCount(0), nMaxTaskCount(nMaxTaskCountIn), nIncCount(nIncCountIn), CMthQueue<T>(ui32QueueSize) {}
    ~CMoreTaskQueue() {}

    /* ui32Timeout is milliseconds */
    virtual bool SetData(T& data, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetData(lock, data, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    virtual bool SetData(const T& data, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetData(lock, data, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool SetMoreData(const std::vector<T>& vDataList, bool& fOpenTask, const uint32 ui32Timeout = 0)
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtSetMoreData(lock, vDataList, ui32Timeout))
        {
            return false;
        }
        fOpenTask = CheckTask();
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetData(T& data, const uint32 ui32Timeout = 0) override
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtGetData(lock, data, ui32Timeout))
        {
            CancelTask();
            return false;
        }
        return true;
    }

    /* ui32Timeout is milliseconds */
    virtual bool GetMoreData(std::vector<T>& vDataList, const uint32 nMaxGetCount = 0, const uint32 ui32Timeout = 0) override
    {
        boost::unique_lock<boost::mutex> lock(CMthQueue<T>::lockQueue);
        if (!CMthQueue<T>::PrtGetMoreData(lock, vDataList, nMaxGetCount, ui32Timeout))
        {
            CancelTask();
            return false;
        }
        return true;
    }

    void SetMaxTaskCount(const uint32 nMaxs)
    {
        nMaxTaskCount = nMaxs;
    }
    uint32 GetMaxTaskCount() const
    {
        return nMaxTaskCount;
    }
    void SetIncCount(const uint64 nIns)
    {
        nIncCount = nIns;
    }
    uint64 GetIncCount() const
    {
        return nIncCount;
    }

protected:
    virtual bool CheckTask()
    {
        if (nRunTaskCount < nMaxTaskCount && (uint32)(CMthQueue<T>::qData.size() / nIncCount + 1) > nRunTaskCount)
        {
            nRunTaskCount++;
            return true;
        }
        return false;
    }
    virtual void CancelTask()
    {
        nRunTaskCount--;
    }

protected:
    uint32 nRunTaskCount;
    uint32 nMaxTaskCount;
    uint64 nIncCount;
};

} // namespace mtbase

#endif // __HSM_MTHBASE_H
