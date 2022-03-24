#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <mpark/variant.hpp>

#include <nodable/core/types.h>
#include <nodable/core/reflection/R.h>
#include <nodable/core/Variant.h>

namespace Nodable
{
    // forward declarations
    class Node;
    class Scope;
    class Member;
    class VariableNode;
    class InstructionNode;

    namespace Asm // Assembly namespace
    {
        /**
         * Enum to identify each register, we try here to follow the x86_64 DASM reference from
         * @see https://www.cs.uaf.edu/2017/fall/cs301/reference/x86_64.html
         */
        enum class Register : u8 {
            rax = 0, // accumulator
            rdx,     // storage
            eip,     // The instruction pointer.
            COUNT
        };

        R_ENUM(Register)
        R_ENUM_VALUE(rax)
        R_ENUM_VALUE(rdx)
        R_ENUM_VALUE(eip)
        R_ENUM_END

        /**
         * Enumerate each possible instruction.
         */
        enum class Instr_t: u8
        {
            cmp, /* compare */
            jmp,
            jne,
            mov,
            pop_stack_frame,
            pop_var,
            push_stack_frame,
            push_var,
            ret
        };

        R_ENUM(Instr_t)
        R_ENUM_VALUE(mov)
        R_ENUM_VALUE(push_var)
        R_ENUM_VALUE(pop_var)
        R_ENUM_VALUE(push_stack_frame)
        R_ENUM_VALUE(pop_stack_frame)
        R_ENUM_VALUE(jmp)
        R_ENUM_VALUE(jne)
        R_ENUM_VALUE(ret)
        R_ENUM_VALUE(cmp)
        R_ENUM_END

        /**
         * Store a single assembly instruction ( line type larg rarg comment )
         */
        struct Instr
        {
            Instr(Instr_t _type, long _line)
                : type(_type)
                , line(_line)
            {}

            u64         line;
            Instr_t     type;
            union {

                struct {
                    i64 offset;
                } jmp;

                struct {
                    Register dst;
                    Register src;
                } mov;

                struct {
                    Register left;
                    Register right;
                } cmp; // compare

                struct {
                    union {
                        VariableNode* var;
                        Scope*        scope;
                    };
                } push;

                struct {
                    union {
                        VariableNode* var;
                        Scope*        scope;
                    };
                } pop;

                struct {
                    Node*    node;
                } eval;

                struct {
                    const Variant* data;
                } store;
            };
            std::string m_comment;
            static std::string to_string(const Instr&);
        };

        /**
         * Wraps an Instruction vector, plus some shortcuts.
         */
        class Code
        {
        public:
            struct MetaData {
                Node* root_node;
            };

            Code(Node* _root): m_meta_data({_root}) {};
            ~Code();

            Instr*                     push_instr(Instr_t _type);
            inline size_t              size() const { return  m_instructions.size(); }
            inline Instr*              get_instruction_at(size_t _index) const { return  m_instructions.at(_index); }
            size_t                     get_next_index() const { return m_instructions.size(); }
            const std::vector<Instr*>& get_instructions()const { return m_instructions; }
            const MetaData&            get_meta_data()const { return m_meta_data; }
            static std::string         to_string(const Code*);
        private:
            MetaData            m_meta_data;
            std::vector<Instr*> m_instructions;
        };

        /**
         * @brief Class to convert a program graph to Assembly::Code
         */
        class Compiler
        {
        public:
            Compiler()= default;
            std::unique_ptr<const Code> compile(Node* _program_graph);
        private:
            void compile_graph_root(Node *_program_graph_root);
            bool is_program_valid(const Node*);
            void compile_node(const Node*);
            void compile_member(const Member*);
            void compile_scope(const Scope*, bool _insert_fake_return = false);
            std::unique_ptr<Code> m_temp_code;

        };
    }
}
