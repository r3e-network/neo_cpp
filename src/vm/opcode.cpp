#include <neo/vm/opcode.h>

#include <string>
#include <unordered_map>

namespace neo::vm
{
std::string GetOpCodeName(OpCode opcode)
{
    static const std::unordered_map<OpCode, std::string> opcode_names = {{OpCode::PUSHINT8, "PUSHINT8"},
                                                                         {OpCode::PUSHINT16, "PUSHINT16"},
                                                                         {OpCode::PUSHINT32, "PUSHINT32"},
                                                                         {OpCode::PUSHINT64, "PUSHINT64"},
                                                                         {OpCode::PUSHINT128, "PUSHINT128"},
                                                                         {OpCode::PUSHINT256, "PUSHINT256"},
                                                                         {OpCode::PUSHT, "PUSHT"},
                                                                         {OpCode::PUSHF, "PUSHF"},
                                                                         {OpCode::PUSHA, "PUSHA"},
                                                                         {OpCode::PUSHNULL, "PUSHNULL"},
                                                                         {OpCode::PUSHDATA1, "PUSHDATA1"},
                                                                         {OpCode::PUSHDATA2, "PUSHDATA2"},
                                                                         {OpCode::PUSHDATA4, "PUSHDATA4"},
                                                                         {OpCode::PUSHM1, "PUSHM1"},
                                                                         {OpCode::PUSH0, "PUSH0"},
                                                                         {OpCode::PUSH1, "PUSH1"},
                                                                         {OpCode::PUSH2, "PUSH2"},
                                                                         {OpCode::PUSH3, "PUSH3"},
                                                                         {OpCode::PUSH4, "PUSH4"},
                                                                         {OpCode::PUSH5, "PUSH5"},
                                                                         {OpCode::PUSH6, "PUSH6"},
                                                                         {OpCode::PUSH7, "PUSH7"},
                                                                         {OpCode::PUSH8, "PUSH8"},
                                                                         {OpCode::PUSH9, "PUSH9"},
                                                                         {OpCode::PUSH10, "PUSH10"},
                                                                         {OpCode::PUSH11, "PUSH11"},
                                                                         {OpCode::PUSH12, "PUSH12"},
                                                                         {OpCode::PUSH13, "PUSH13"},
                                                                         {OpCode::PUSH14, "PUSH14"},
                                                                         {OpCode::PUSH15, "PUSH15"},
                                                                         {OpCode::PUSH16, "PUSH16"},
                                                                         {OpCode::NOP, "NOP"},
                                                                         {OpCode::JMP, "JMP"},
                                                                         {OpCode::JMP_L, "JMP_L"},
                                                                         {OpCode::JMPIF, "JMPIF"},
                                                                         {OpCode::JMPIF_L, "JMPIF_L"},
                                                                         {OpCode::JMPIFNOT, "JMPIFNOT"},
                                                                         {OpCode::JMPIFNOT_L, "JMPIFNOT_L"},
                                                                         {OpCode::JMPEQ, "JMPEQ"},
                                                                         {OpCode::JMPEQ_L, "JMPEQ_L"},
                                                                         {OpCode::JMPNE, "JMPNE"},
                                                                         {OpCode::JMPNE_L, "JMPNE_L"},
                                                                         {OpCode::JMPGT, "JMPGT"},
                                                                         {OpCode::JMPGT_L, "JMPGT_L"},
                                                                         {OpCode::JMPGE, "JMPGE"},
                                                                         {OpCode::JMPGE_L, "JMPGE_L"},
                                                                         {OpCode::JMPLT, "JMPLT"},
                                                                         {OpCode::JMPLT_L, "JMPLT_L"},
                                                                         {OpCode::JMPLE, "JMPLE"},
                                                                         {OpCode::JMPLE_L, "JMPLE_L"},
                                                                         {OpCode::CALL, "CALL"},
                                                                         {OpCode::CALL_L, "CALL_L"},
                                                                         {OpCode::CALLA, "CALLA"},
                                                                         {OpCode::CALLT, "CALLT"},
                                                                         {OpCode::ABORT, "ABORT"},
                                                                         {OpCode::ASSERT, "ASSERT"},
                                                                         {OpCode::THROW, "THROW"},
                                                                         {OpCode::TRY, "TRY"},
                                                                         {OpCode::TRY_L, "TRY_L"},
                                                                         {OpCode::ENDTRY, "ENDTRY"},
                                                                         {OpCode::ENDTRY_L, "ENDTRY_L"},
                                                                         {OpCode::ENDFINALLY, "ENDFINALLY"},
                                                                         {OpCode::RET, "RET"},
                                                                         {OpCode::SYSCALL, "SYSCALL"},
                                                                         {OpCode::DEPTH, "DEPTH"},
                                                                         {OpCode::DROP, "DROP"},
                                                                         {OpCode::NIP, "NIP"},
                                                                         {OpCode::XDROP, "XDROP"},
                                                                         {OpCode::CLEAR, "CLEAR"},
                                                                         {OpCode::DUP, "DUP"},
                                                                         {OpCode::OVER, "OVER"},
                                                                         {OpCode::PICK, "PICK"},
                                                                         {OpCode::TUCK, "TUCK"},
                                                                         {OpCode::SWAP, "SWAP"},
                                                                         {OpCode::ROT, "ROT"},
                                                                         {OpCode::ROLL, "ROLL"},
                                                                         {OpCode::REVERSE3, "REVERSE3"},
                                                                         {OpCode::REVERSE4, "REVERSE4"},
                                                                         {OpCode::REVERSEN, "REVERSEN"},
                                                                         {OpCode::INITSSLOT, "INITSSLOT"},
                                                                         {OpCode::INITSLOT, "INITSLOT"},
                                                                         {OpCode::LDSFLD0, "LDSFLD0"},
                                                                         {OpCode::LDSFLD1, "LDSFLD1"},
                                                                         {OpCode::LDSFLD2, "LDSFLD2"},
                                                                         {OpCode::LDSFLD3, "LDSFLD3"},
                                                                         {OpCode::LDSFLD4, "LDSFLD4"},
                                                                         {OpCode::LDSFLD5, "LDSFLD5"},
                                                                         {OpCode::LDSFLD6, "LDSFLD6"},
                                                                         {OpCode::LDSFLD, "LDSFLD"},
                                                                         {OpCode::STSFLD0, "STSFLD0"},
                                                                         {OpCode::STSFLD1, "STSFLD1"},
                                                                         {OpCode::STSFLD2, "STSFLD2"},
                                                                         {OpCode::STSFLD3, "STSFLD3"},
                                                                         {OpCode::STSFLD4, "STSFLD4"},
                                                                         {OpCode::STSFLD5, "STSFLD5"},
                                                                         {OpCode::STSFLD6, "STSFLD6"},
                                                                         {OpCode::STSFLD, "STSFLD"},
                                                                         {OpCode::LDLOC0, "LDLOC0"},
                                                                         {OpCode::LDLOC1, "LDLOC1"},
                                                                         {OpCode::LDLOC2, "LDLOC2"},
                                                                         {OpCode::LDLOC3, "LDLOC3"},
                                                                         {OpCode::LDLOC4, "LDLOC4"},
                                                                         {OpCode::LDLOC5, "LDLOC5"},
                                                                         {OpCode::LDLOC6, "LDLOC6"},
                                                                         {OpCode::LDLOC, "LDLOC"},
                                                                         {OpCode::STLOC0, "STLOC0"},
                                                                         {OpCode::STLOC1, "STLOC1"},
                                                                         {OpCode::STLOC2, "STLOC2"},
                                                                         {OpCode::STLOC3, "STLOC3"},
                                                                         {OpCode::STLOC4, "STLOC4"},
                                                                         {OpCode::STLOC5, "STLOC5"},
                                                                         {OpCode::STLOC6, "STLOC6"},
                                                                         {OpCode::STLOC, "STLOC"},
                                                                         {OpCode::LDARG0, "LDARG0"},
                                                                         {OpCode::LDARG1, "LDARG1"},
                                                                         {OpCode::LDARG2, "LDARG2"},
                                                                         {OpCode::LDARG3, "LDARG3"},
                                                                         {OpCode::LDARG4, "LDARG4"},
                                                                         {OpCode::LDARG5, "LDARG5"},
                                                                         {OpCode::LDARG6, "LDARG6"},
                                                                         {OpCode::LDARG, "LDARG"},
                                                                         {OpCode::STARG0, "STARG0"},
                                                                         {OpCode::STARG1, "STARG1"},
                                                                         {OpCode::STARG2, "STARG2"},
                                                                         {OpCode::STARG3, "STARG3"},
                                                                         {OpCode::STARG4, "STARG4"},
                                                                         {OpCode::STARG5, "STARG5"},
                                                                         {OpCode::STARG6, "STARG6"},
                                                                         {OpCode::STARG, "STARG"},
                                                                         {OpCode::NEWBUFFER, "NEWBUFFER"},
                                                                         {OpCode::MEMCPY, "MEMCPY"},
                                                                         {OpCode::CAT, "CAT"},
                                                                         {OpCode::SUBSTR, "SUBSTR"},
                                                                         {OpCode::LEFT, "LEFT"},
                                                                         {OpCode::RIGHT, "RIGHT"},
                                                                         {OpCode::SIZE, "SIZE"},
                                                                         {OpCode::INVERT, "INVERT"},
                                                                         {OpCode::AND, "AND"},
                                                                         {OpCode::OR, "OR"},
                                                                         {OpCode::XOR, "XOR"},
                                                                         {OpCode::EQUAL, "EQUAL"},
                                                                         {OpCode::NOTEQUAL, "NOTEQUAL"},
                                                                         {OpCode::SIGN, "SIGN"},
                                                                         {OpCode::ABS, "ABS"},
                                                                         {OpCode::NEGATE, "NEGATE"},
                                                                         {OpCode::INC, "INC"},
                                                                         {OpCode::DEC, "DEC"},
                                                                         {OpCode::ADD, "ADD"},
                                                                         {OpCode::SUB, "SUB"},
                                                                         {OpCode::MUL, "MUL"},
                                                                         {OpCode::DIV, "DIV"},
                                                                         {OpCode::MOD, "MOD"},
                                                                         {OpCode::POW, "POW"},
                                                                         {OpCode::SQRT, "SQRT"},
                                                                         {OpCode::MODMUL, "MODMUL"},
                                                                         {OpCode::MODPOW, "MODPOW"},
                                                                         {OpCode::SHL, "SHL"},
                                                                         {OpCode::SHR, "SHR"},
                                                                         {OpCode::NOT, "NOT"},
                                                                         {OpCode::BOOLAND, "BOOLAND"},
                                                                         {OpCode::BOOLOR, "BOOLOR"},
                                                                         {OpCode::NZ, "NZ"},
                                                                         {OpCode::NUMEQUAL, "NUMEQUAL"},
                                                                         {OpCode::NUMNOTEQUAL, "NUMNOTEQUAL"},
                                                                         {OpCode::LT, "LT"},
                                                                         {OpCode::LE, "LE"},
                                                                         {OpCode::GT, "GT"},
                                                                         {OpCode::GE, "GE"},
                                                                         {OpCode::MIN, "MIN"},
                                                                         {OpCode::MAX, "MAX"},
                                                                         {OpCode::WITHIN, "WITHIN"},
                                                                         {OpCode::PACKMAP, "PACKMAP"},
                                                                         {OpCode::PACKSTRUCT, "PACKSTRUCT"},
                                                                         {OpCode::PACK, "PACK"},
                                                                         {OpCode::UNPACK, "UNPACK"},
                                                                         {OpCode::NEWARRAY0, "NEWARRAY0"},
                                                                         {OpCode::NEWARRAY, "NEWARRAY"},
                                                                         {OpCode::NEWARRAY_T, "NEWARRAY_T"},
                                                                         {OpCode::NEWSTRUCT0, "NEWSTRUCT0"},
                                                                         {OpCode::NEWSTRUCT, "NEWSTRUCT"},
                                                                         {OpCode::NEWMAP, "NEWMAP"},
                                                                         {OpCode::APPEND, "APPEND"},
                                                                         {OpCode::SETITEM, "SETITEM"},
                                                                         {OpCode::PICKITEM, "PICKITEM"},
                                                                         {OpCode::REMOVE, "REMOVE"},
                                                                         {OpCode::CLEARITEMS, "CLEARITEMS"},
                                                                         {OpCode::POPITEM, "POPITEM"},
                                                                         {OpCode::ISNULL, "ISNULL"},
                                                                         {OpCode::ISTYPE, "ISTYPE"},
                                                                         {OpCode::CONVERT, "CONVERT"},
                                                                         {OpCode::ABORTMSG, "ABORTMSG"},
                                                                         {OpCode::ASSERTMSG, "ASSERTMSG"}};

    auto it = opcode_names.find(opcode);
    if (it != opcode_names.end())
    {
        return it->second;
    }

    return "UNKNOWN";
}

bool IsPushOp(OpCode opcode)
{
    return (opcode >= OpCode::PUSHINT8 && opcode <= OpCode::PUSHINT256) ||
           (opcode >= OpCode::PUSH0 && opcode <= OpCode::PUSH16) || opcode == OpCode::PUSHM1 ||
           opcode == OpCode::PUSHA || opcode == OpCode::PUSHNULL || opcode == OpCode::PUSHDATA1 ||
           opcode == OpCode::PUSHDATA2 || opcode == OpCode::PUSHDATA4 || opcode == OpCode::PUSHT ||
           opcode == OpCode::PUSHF;
}

int GetPushOpOperandSize(OpCode opcode)
{
    switch (opcode)
    {
        case OpCode::PUSHINT8:
            return 1;
        case OpCode::PUSHINT16:
            return 2;
        case OpCode::PUSHINT32:
            return 4;
        case OpCode::PUSHINT64:
            return 8;
        case OpCode::PUSHINT128:
            return 16;
        case OpCode::PUSHINT256:
            return 32;
        case OpCode::PUSHA:
            return 4;
        case OpCode::PUSHDATA1:
            return 1;
        case OpCode::PUSHDATA2:
            return 2;
        case OpCode::PUSHDATA4:
            return 4;
        default:
            return 0;
    }
}
}  // namespace neo::vm
