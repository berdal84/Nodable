#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <memory>

#include <nodable/core/reflection/type.h>
#include <nodable/core/constants.h>

namespace Nodable
{
    // forward declarations
    class Operator;

    /*
     * Simple object to store a named function argument
     */
    struct FuncArg
    {
        FuncArg(type _type, std::string& _name): m_type(_type), m_name(_name){}
        type        m_type;
        std::string m_name;
    };

    using FuncArgs = std::vector<FuncArg>;

    /*
     * Class to store a function signature.
     * We can check if two function signature are matching using this->match(other)
     */
    class func_type
    {
    public:
        func_type(std::string _id);
        func_type(std::string _id, const Operator* _op);
        ~func_type() {};

        void                           push_arg(type _type);

        template <typename... T>
        void push_args(T&&... args) {
            int dummy[] = { 0, ((void) push_arg(std::forward<T>(args)),0)... };
        }

        bool                           has_an_arg_of_type(type type)const;
        bool                           is_exactly(const func_type* _other)const;
        bool                           is_compatible(const func_type* _other)const;
        bool                           is_operator()const { return m_operator; };
        const std::string&             get_identifier()const { return m_identifier; };
        FuncArgs&                      get_args() { return m_args;};
        const FuncArgs&                get_args()const { return m_args;};
        size_t                         get_arg_count() const { return m_args.size(); }
        const type                     get_return_type() const { return m_return_type; }
        void                           set_return_type(type _type) { m_return_type = _type; };
        const Operator*                get_operator()const { return m_operator; }
        std::string                    get_label()const;
    private:
        const Operator* m_operator;
        std::string     m_identifier;
        FuncArgs        m_args;
        type            m_return_type;

    public:

        /** Push Arg helpers */

        template<class Tuple, std::size_t N> // push N+1 arguments
        struct arg_pusher
        {
            static void push_into(func_type *_signature)
            {
                arg_pusher<Tuple, N - 1>::push_into(_signature);

                using T = std::tuple_element_t<N-1, Tuple>;
                _signature->push_arg( type::get<T>() );
            }
        };

        template<class Tuple>  // push 1 arguments
        struct arg_pusher<Tuple, 1>
        {
            static void push_into(func_type *_signature)
            {
                using T = std::tuple_element_t<0, Tuple>;
                _signature->push_arg( type::get<T>() );
            };
        };

        // create an argument_pusher and push arguments into signature
        template<typename... Args, std::enable_if_t<std::tuple_size_v<Args...> != 0, int> = 0>
        void push_args()
        {
            arg_pusher<Args..., std::tuple_size_v<Args...>>::push_into(this);
        }

        // empty function when pushing an empty arguments
        template<typename... Args, std::enable_if_t<std::tuple_size_v<Args...> == 0, int> = 0>
        void push_args(){}
    };


}