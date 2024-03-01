// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_LEVELDBENG_H
#define STORAGE_LEVELDBENG_H
#include <boost/filesystem/path.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "mtbase.h"

namespace metabasenet
{
namespace storage
{

class CLevelDBArguments
{
public:
    CLevelDBArguments();
    ~CLevelDBArguments();

public:
    std::string path;
    size_t cache;
    bool syncwrite;
    int files;
};

class CLevelDBEngine : public mtbase::CKVDBEngine
{
public:
    CLevelDBEngine(CLevelDBArguments& arguments);
    ~CLevelDBEngine();

    bool Open() override;
    void Close() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    void TxnAbort() override;
    bool Get(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue) override;
    bool Put(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue, bool fOverwrite) override;
    bool Remove(mtbase::CBufStream& ssKey) override;
    bool RemoveAll() override;
    bool MoveFirst() override;
    bool MoveTo(mtbase::CBufStream& ssKey) override;
    bool MoveNext(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue) override;

protected:
    std::string path;
    leveldb::DB* pdb;
    leveldb::Iterator* piter;
    leveldb::WriteBatch* pbatch;
    leveldb::Options options;
    leveldb::ReadOptions readoptions;
    leveldb::WriteOptions writeoptions;
    leveldb::WriteOptions batchoptions;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_LEVELDBENG_H
