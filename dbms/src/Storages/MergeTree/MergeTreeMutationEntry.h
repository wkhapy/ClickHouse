#pragma once

#include <Common/Exception.h>
#include <Core/Types.h>
#include <IO/WriteHelpers.h>
#include <Storages/MutationCommands.h>


namespace DB
{

class ReadBuffer;
class WriteBuffer;

struct MergeTreeMutationEntry
{
    void writeText(WriteBuffer & out) const;
    void readText(ReadBuffer & in);

    String toString() const;
    static MergeTreeMutationEntry parse(const String & str, String id);

    /// For Replicated tables id is a znode name in ZooKeeper.
    String id;

    time_t create_time = 0;
    /// Empty for non-replicated tables.
    String source_replica;

    /// For non-replicated tables the map is single entry "" -> block_number.
    std::map<String, Int64> block_numbers;
    MutationCommands commands;
};

using MergeTreeMutationEntryPtr = std::shared_ptr<const MergeTreeMutationEntry>;

}
