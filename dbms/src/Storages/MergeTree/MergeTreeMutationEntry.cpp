#include <Storages/MergeTree/MergeTreeMutationEntry.h>
#include <Parsers/ParserAlterQuery.h>
#include <Parsers/parseQuery.h>
#include <Parsers/formatAST.h>
#include <IO/Operators.h>
#include <IO/ReadBufferFromString.h>
#include <IO/WriteBufferFromString.h>
#include <Common/typeid_cast.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int UNKNOWN_MUTATION_COMMAND;
}

void MergeTreeMutationEntry::writeText(WriteBuffer & out) const
{
    out << "format version: 1\n"
        << "create time: " << LocalDateTime(create_time ? create_time : time(nullptr)) << "\n"
        << "source replica: " << source_replica << "\n"
        << "block numbers count: " << block_numbers.size() << "\n";

    for (const auto & kv : block_numbers)
    {
        const String & partition_id = kv.first;
        Int64 number = kv.second;
        out << partition_id << "\t" << number << "\n";
    }

    std::stringstream commands_ss;
    formatAST(*commands.ast(), commands_ss, /* hilite = */ false, /* one_line = */ true);
    out << "commands: " << escape << commands_ss.str();
}

void MergeTreeMutationEntry::readText(ReadBuffer & in)
{
    in >> "format version: 1\n";

    LocalDateTime create_time_dt;
    in >> "create time: " >> create_time_dt >> "\n";
    create_time = create_time_dt;

    in >> "source replica: " >> source_replica >> "\n";

    size_t count;
    in >> "block numbers count: " >> count >> "\n";
    for (size_t i = 0; i < count; ++i)
    {
        String partition_id;
        Int64 number;
        in >> partition_id >> "\t" >> number >> "\n";
        block_numbers[partition_id] = number;
    }

    String commands_str;
    in >> "commands: " >> escape >> commands_str;

    ParserAlterCommandList p_alter_commands;
    auto commands_ast = parseQuery(
        p_alter_commands, commands_str.data(), commands_str.data() + commands_str.length(), "mutation commands list", 0);
    for (ASTAlterCommand * command_ast : typeid_cast<const ASTAlterCommandList &>(*commands_ast).commands)
    {
        auto command = MutationCommand::parse(command_ast);
        if (!command)
            throw Exception("Unknown mutation command type: " + DB::toString<int>(command_ast->type), ErrorCodes::UNKNOWN_MUTATION_COMMAND);
        commands.push_back(std::move(*command));
    }

}

String MergeTreeMutationEntry::toString() const
{
    WriteBufferFromOwnString out;
    writeText(out);
    return out.str();
}

MergeTreeMutationEntry MergeTreeMutationEntry::parse(const String & str, String id)
{
    MergeTreeMutationEntry res;
    res.id = std::move(id);

    ReadBufferFromString in(str);
    res.readText(in);
    assertEOF(in);

    return res;
}

}
