#pragma once

#include <string>
#include <memory>

#include <nodable/core/types.h> // forward declarations and common stuff
#include <nodable/core/Node.h> // base class
#include <nodable/core/Member.h>

namespace Nodable
{
    // forward declarations
    struct Token;

    /*
        The role of this class is to symbolize an instruction.
        The result of the instruction is value()
    */
    class InstructionNode : public Node
    {
    public:
        explicit InstructionNode(const char* _label);
        ~InstructionNode()= default;

        [[nodiscard]] inline Member* get_root_node_member()const { return m_props.get("root_node"); }
        [[nodiscard]] inline std::shared_ptr<Token> end_of_instr_token()const { return m_end_of_instr_token; }
                      inline void    end_of_instr_token(std::shared_ptr<Token> token) { m_end_of_instr_token = token; }

    private:
        std::shared_ptr<Token> m_end_of_instr_token = nullptr;
        R_DERIVED(InstructionNode)
        R_EXTENDS(Node)
        R_END
    };
}