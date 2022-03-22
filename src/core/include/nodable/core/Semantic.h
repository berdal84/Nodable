#pragma once

#include <string>
#include <map>
#include <vector>
#include <regex>
#include <memory> // std::shared_ptr

#include <nodable/core/types.h>
#include <nodable/core/TokenType.h>
#include <nodable/core/reflection/R.h>

namespace Nodable
{
	/**
	* The role of Semantic is to provide a way to insert and apply simple conversions:
	 *
	 * std::string   <===> TokenType
	 * std::regex    <===> TokenType
	 * Nodable::Type <===> TokenType
	 *
	 * A Semantic is designed to a part of a Language.
	*/
	class Semantic
	{
	public:
		Semantic();
		~Semantic() = default;

		/**
		 * Insert a regular expression to TokenType correspondence (for a Parser).
		 *
		 * A string that matching with the regular expression will be interpreted as a specific given type.
		 */
		void insert(const std::regex&, TokenType);
        void insert(const std::regex&, TokenType, R::Type);

		/**
		 * Insert a string to TokenType correspondence.
		 *
		 * This will provide:
		 * - a TokenType/std::string simple correspondence (for a Serializer).
		 * - a std::regex/TokenType correspondence (for a Parser).
		 */
		void insert(std::string, TokenType);
        void insert(std::string, TokenType, R::Type);

		/**
		 * Insert a bidirectional correspondence between a type and a token type (for a Language)
		 */
		void insert(std::string, R::Type);


		[[nodiscard]] inline const std::vector<std::regex>& get_token_type_regex()const { return m_token_type_regex;  }
		[[nodiscard]] inline const std::vector<std::regex>& get_type_regex()const { return m_type_regex;  }
        [[nodiscard]] inline std::string                    type_to_string(std::shared_ptr<const R::MetaType> _type)const { return m_type_to_string[ static_cast<size_t>(_type->get_type()) ]; }
        [[nodiscard]] inline std::string                    token_type_to_string(TokenType _type)const { return m_token_type_to_string[static_cast<size_t>(_type)]; }
        [[nodiscard]] inline R::Type                    token_type_to_type(TokenType _tokenType)const { return m_token_type_to_type[_tokenType]; }
        [[nodiscard]] inline const std::vector<TokenType>&  get_token_type_regex_index_to_token_type()const { return m_regex_index_to_token_type; }
        [[nodiscard]] inline const std::vector<R::Type>&    get_type_regex_index_to_type()const { return m_regex_index_to_type; }

    private:
	    /**
	     * Two vector to:
	     *  1- iterate fast when we parse using all regex (in m_regex)
	     *  2- once we found a regex that matches we get the TokenType (located at same index in m_type)
	     */
        std::vector<std::regex>  m_type_regex;
        std::vector<std::regex>  m_token_type_regex;

        std::vector<TokenType>   m_regex_index_to_token_type;
        std::vector<R::Type> m_regex_index_to_type;

		std::vector<TokenType>   m_type_to_token_type;
        std::vector<R::Type> m_token_type_to_type;

        std::vector<std::string> m_token_type_to_string;
        std::vector<std::string> m_type_to_string;
    };
}