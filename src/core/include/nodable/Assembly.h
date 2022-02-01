#pragma once

#include <string>
#include <vector>
#include <mpark/variant.hpp>
#include <nodable/Nodable.h>

namespace Nodable
{
    // forward declarations
    class ScopeNode;
    class Node;
    class Member;

    namespace Asm // Assembly namespace
    {
        /**
         * Enum to identify each register, we try here to follow the x86_64 DASM reference from
         * @see https://www.cs.uaf.edu/2017/fall/cs301/reference/x86_64.html
         */
        enum Register {
            rax = 0, // accumulator
            rdx,     // storage
            esp,     // The stack pointer.  Points to the top of the stack
            ebp,     // Preserved register. Sometimes used to store the old value of the stack pointer, or the "base".
            COUNT
        };


        /**
         * Enum to identify each function identifier.
         * A function is specified when using "call" instruction.
         */
        enum class FctId: i64_t
        {
            eval_member
        };

        /**
         * Enumerate each possible instruction.
         */
        enum class Instr_t: i8_t
        {
            call,
            mov,
            jmp,
            jne,
            ret,
            cmp /* compare */
        };

        /**
         * Store a single assembly instruction ( line type larg rarg comment )
         */
        struct Instr
        {
            Instr(Instr_t _type, long _line): m_type(_type), m_line(_line) {}

            i64_t   m_line;
            Instr_t m_type;
            i64_t   m_left_h_arg;
            i64_t   m_right_h_arg;
            std::string m_comment;
            static std::string to_string(const Instr&);
        };

        /**
         * Wraps an Instruction vector, plus some shortcuts.
         */
        class Code
        {
        public:
            Code() = default;
            ~Code();

            Instr*        push_instr(Instr_t _type);
            inline size_t size() const { return  m_instructions.size(); }
            inline Instr* operator[](size_t _index) const { return  m_instructions[_index]; }
            long          get_next_pushed_instr_index() const { return m_instructions.size(); }
            const std::vector<Instr*>& get_instructions()const { return m_instructions; }
        private:
            std::vector<Instr*> m_instructions;
        };

        /**
         * Class to check a program node and convert it to Assembly::Code
         */
        class Compiler
        {
        public:
            Compiler():m_output(nullptr){}
            bool          create_assembly_code(const ScopeNode* _program);
            void          append_to_assembly_code(const Node* _node);
            Code*         get_output_assembly();
            static bool   is_program_valid(const ScopeNode* _program);
        private:
            Code* m_output;
        };
    }

    static std::string to_string(Asm::Register);
    static std::string to_string(Asm::FctId);
    static std::string to_string(Asm::Instr_t);
}