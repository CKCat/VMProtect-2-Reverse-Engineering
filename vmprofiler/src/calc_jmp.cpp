#include <vmprofiler.hpp>

namespace vm::calc_jmp
{
    bool get( zydis_routine_t &vm_entry, zydis_routine_t &calc_jmp )
    {
        auto result = vm::util::get_fetch_operand( vm_entry );

        if ( !result.has_value() )
            return false;

        calc_jmp.insert( calc_jmp.end(), result.value(), vm_entry.end() );
        return true;
    }

    std::optional< vmp2::exec_type_t > get_advancement( const zydis_routine_t &calc_jmp )
    {
        auto result = std::find_if( calc_jmp.begin(), calc_jmp.end(), []( const zydis_instr_t &instr_data ) -> bool {
            // look for any instruction with RSI being the first operand and its being written too..
            return instr_data.instr.operands[ 0 ].actions & ZYDIS_OPERAND_ACTION_WRITE &&
                   instr_data.instr.operands[ 0 ].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   instr_data.instr.operands[ 0 ].reg.value == ZYDIS_REGISTER_RSI;
        } );

        if ( result == calc_jmp.end() )
            return {};

        const auto &instr = result->instr;

        switch ( instr.mnemonic )
        {
        case ZYDIS_MNEMONIC_LEA:
            // if operand type is memory, then return advancement type
            // based off of the disposition value... (neg == backward, pos == forward)
            if ( instr.operands[ 1 ].type == ZYDIS_OPERAND_TYPE_MEMORY )
                return instr.operands[ 1 ].mem.disp.value > 0 ? vmp2::exec_type_t::forward
                                                              : vmp2::exec_type_t::backward;
            break;
        case ZYDIS_MNEMONIC_ADD:
            if ( instr.operands[ 1 ].type == ZYDIS_OPERAND_TYPE_IMMEDIATE )
                return instr.operands[ 1 ].imm.value.s > 0 ? vmp2::exec_type_t::forward : vmp2::exec_type_t::backward;
            break;
        case ZYDIS_MNEMONIC_SUB:
            if ( instr.operands[ 1 ].type == ZYDIS_OPERAND_TYPE_IMMEDIATE )
                return instr.operands[ 1 ].imm.value.s > 0 ? vmp2::exec_type_t::backward : vmp2::exec_type_t::forward;
            break;
        case ZYDIS_MNEMONIC_INC:
            return vmp2::exec_type_t::forward;
        case ZYDIS_MNEMONIC_DEC:
            return vmp2::exec_type_t::backward;
        default:
            break;
        }
        return {};
    }
} // namespace vm::calc_jmp