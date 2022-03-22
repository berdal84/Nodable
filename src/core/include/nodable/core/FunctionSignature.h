#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <memory>

#include <nodable/core/reflection/R.h>

namespace Nodable
{

    /*
     * Simple object to store a named function argument
     */
    struct FunctionArg
    {
        FunctionArg(std::shared_ptr<const R::MetaType> _type, std::string& _name): m_type(_type), m_name(_name){}
        std::shared_ptr<const R::MetaType> m_type;
        std::string m_name;
    };

    /*
     * Class to store a function signature.
     * We can check if two function signature are matching using this->match(other)
     */
    class FunctionSignature {
    public:
        FunctionSignature(std::string _identifier, std::string _label = "");
        ~FunctionSignature() {};
        void                           push_arg(std::shared_ptr<const R::MetaType> _type, std::string _name = "");

        template <typename... T>
        void push_args(T&&... args) {
            int dummy[] = { 0, ((void) push_arg(std::forward<T>(args)),0)... };
        }

        bool                           has_an_arg_of_type(std::shared_ptr<const R::MetaType> type)const;
        bool                           match(const FunctionSignature* _other)const;
        const std::string&             get_identifier()const;
        std::vector<FunctionArg>       get_args() const;
        size_t                         get_arg_count() const { return m_args.size(); }
        std::shared_ptr<const R::MetaType> get_return_type() const;
        void                           set_return_type(std::shared_ptr<const R::MetaType> _type) { m_return_type = _type; };
        std::string                    get_label() const;

    private:
        std::string m_label;
        std::string m_identifier;
        std::shared_ptr<const R::MetaType> m_return_type;
        std::vector<FunctionArg> m_args;

    public:

        /** helpers to create a FunctionSignature */

        template<typename T>
        struct new_instance;

        template<typename T, typename... Args>
        struct new_instance<T(Args...)>
        {
            using F = T(Args...);
            static FunctionSignature* with_id(const char* _identifier, const char* _label = "")
            {
                auto signature = new FunctionSignature(_identifier, _label);
                signature->set_return_type(R::get_meta_type<T>() );
                signature->push_args<std::tuple<Args...>>();
                return signature;
            }
        };

        /** Push Arg helpers */

        template<class Tuple, std::size_t N> // push N+1 arguments
        struct arg_pusher
        {
            static void push_into(FunctionSignature *_signature)
            {
                arg_pusher<Tuple, N - 1>::push_into(_signature);

                using T = std::tuple_element_t<N-1, Tuple>;
                _signature->push_arg(R::get_meta_type<T>() );
            }
        };

        template<class Tuple>  // push 1 arguments
        struct arg_pusher<Tuple, 1>
        {
            static void push_into(FunctionSignature *_signature)
            {
                using T = std::tuple_element_t<0, Tuple>;
                _signature->push_arg(R::get_meta_type<T>() );
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