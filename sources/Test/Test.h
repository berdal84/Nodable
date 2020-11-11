#pragma once

#include <memory> // for unique_ptr
#include <stack>
#include <Language/LanguageNodable.h>
#include "Member.h"
#include "Log.h"
#include "Node.h"
#include "Wire.h"
#include "DataAccess.h"
#include "Container.h"
#include "Language.h"
#include "Parser.h"
#include "Variable.h"

struct TestState {
	TestState() {};

	void add(const TestState& _other) {
		this->m_count += _other.m_count;
		this->m_successCount += _other.m_successCount;
		this->m_failureCount += _other.m_failureCount;
	}

	void addSuccess() {
		this->m_successCount++;
		this->m_count++;
	}

	void addFailure() {
		this->m_failureCount++;
		this->m_count++;
	}

	bool allPassed()const {
		return m_count == m_successCount;
	}

	size_t m_count = 0;
	size_t m_successCount = 0;
	size_t m_failureCount = 0;
};

using namespace Nodable;

static TestState s_globalState; \
static bool s_lastGroupTestPassed; \
static std::stack<TestState> status;

#define GLOBAL_TEST_BEGIN \
	s_globalState = TestState(); \
	s_lastGroupTestPassed = false;

#define GLOBAL_TEST_END \
	if(  s_globalState.allPassed() ) { \
		LOG_DEBUG(0u, GREEN"All tests are OK : (%d / %d passed).\n", s_globalState.m_successCount, s_globalState.m_count); \
	} else { \
		LOG_DEBUG(0u, RED"Some tests FAILED !: (%d / %d passed).\n", s_globalState.m_successCount, s_globalState.m_count); \
	}

#define GLOBAL_TEST_RESULT s_globalState.allPassed();

#define TEST_BEGIN(name) \
	{ \
		LOG_MESSAGE(0u, "Testing %s...\n", name);\
		status.push( TestState() ); \
		{

#define TEST_END \
		} \
		if( ! status.top().allPassed() ) { \
			LOG_ERROR(0u, "Test: failed, %i/%i passed.\n", status.top().m_successCount,  status.top().m_count); \
		} \
		s_globalState.add(status.top()); \
		s_lastGroupTestPassed = status.top().allPassed(); \
		status.pop(); \
	} 

#define TEST_RESULT s_globalState.allPassed()

#define EXPECT(function, expected) \
    {auto result = function; \
	if (result == expected) { \
		status.top().addSuccess();       \
        LOG_DEBUG( 0u, #function" : OK ! \n"); \
	} else { \
		LOG_ERROR( 0u, #function" : FAILED ! expected:" #expected ", result: %s\n", result ); \
		status.top().addFailure();\
	}}


bool Member_Connections_Tests() {

	TEST_BEGIN("Member Way"){

		TEST_BEGIN("Member 1: In"){
            auto m = std::make_unique<Member>();
                    m->setAllowedConnections(Way::In);

			EXPECT(m->allowsConnections(Way::Out)	, false)
			EXPECT(m->allowsConnections(Way::InOut)	, false)
			EXPECT(m->allowsConnections(Way::In)	, true)
			EXPECT(m->allowsConnections(Way::None)	, false)
		}TEST_END


		TEST_BEGIN("Member 2: Out"){
            auto m = std::make_unique<Member>();
                    m->setAllowedConnections(Way::Out);

			EXPECT(m->allowsConnections(Way::Out)	, true)
			EXPECT(m->allowsConnections(Way::InOut)	, false)
			EXPECT(m->allowsConnections(Way::In)		, false)
			EXPECT(m->allowsConnections(Way::None)	, false)
		}TEST_END

		TEST_BEGIN("Member 3: Way::None"){
            auto m = std::make_unique<Member>();
                    m->setAllowedConnections(Way::None);

			EXPECT(m->allowsConnections(Way::Out)	, false)
			EXPECT(m->allowsConnections(Way::InOut)	, false)
			EXPECT(m->allowsConnections(Way::In)		, false)
			EXPECT(m->allowsConnections(Way::None)	, true)
		}TEST_END

		TEST_BEGIN("Member 4: Way::InOut"){
		    auto m = std::make_unique<Member>();
                    m->setAllowedConnections(Way::InOut);

			EXPECT(m->allowsConnections(Way::Out)	, true)
			EXPECT(m->allowsConnections(Way::InOut)	, true)
			EXPECT(m->allowsConnections(Way::In)	, true)
			EXPECT(m->allowsConnections(Way::None)	, false)
		}TEST_END

	}TEST_END

	return s_lastGroupTestPassed;
}

bool Member_AsBoolean_Tests() {

	TEST_BEGIN("Member: Booleans"){

        auto m = std::make_unique<Member>();

		m->set(true);
		EXPECT((bool)*m, true)
		EXPECT(m->getType(), Type::Boolean)

		m->set(false);
		EXPECT((bool)*m, false)
		EXPECT(m->isSet(), true)

	}TEST_END

	return s_lastGroupTestPassed;
}

bool Member_AsString_Tests() {

	TEST_BEGIN("Member: String"){

		auto m = std::make_shared<Member>();
		m->set("Hello world !");
		const std::string str = "Hello world !";

		EXPECT((std::string)*m, str)
		EXPECT((bool)*m, true)
		EXPECT(m->getType(), Type::String)
		EXPECT((double)*m, str.length())
		EXPECT(m->isSet(), true)
	}TEST_END

	return s_lastGroupTestPassed;
}

bool Member_AsNumber_Tests() {

	TEST_BEGIN("Member: Double"){
				
		auto m = std::make_unique<Member>();
		m->set((double)50);

		EXPECT((double)*m, (double)50)
		EXPECT(m->getType(), Type::Double)
		EXPECT(m->isSet(), true)	

	}TEST_END

	return s_lastGroupTestPassed;

}

template <typename T>
bool Parser_Test(
	const std::string& expression,	
	T _expectedValue )
{
//    Nodable::Log::SetVerbosityLevel(4u);

    auto language  = std::make_shared<LanguageNodable>();
	auto container = std::make_shared<Container>(language);
	auto parser    = std::make_unique<Parser>(language, container);

	if( parser->eval(expression) ) {
        container->update();

        auto result = container->getResultVariable();
        auto resultMember = result->getMember();

        auto expectedMember = std::make_shared<Member>();
        expectedMember->set(_expectedValue);

        auto success = result->getMember()->equals(expectedMember);

        LOG_MESSAGE( 0u, "Result: %s\n", std::string(*resultMember).c_str() );
        return success;
    }
	else
    {
	    throw std::runtime_error("Unable to eval the expression: " + expression );
    }
}

bool Parser_Tests() {

	TEST_BEGIN("Parser"){

		TEST_BEGIN("Simple"){
			EXPECT( Parser_Test("-5", -5), true)
			EXPECT( Parser_Test("2+3", 5), true)
			EXPECT( Parser_Test("-5+4", -1), true)
			EXPECT( Parser_Test("-1+2*5-3/6", 8.5), true)
		}TEST_END

		TEST_BEGIN("Simple parenthesis"){
			EXPECT( Parser_Test("(1+4)", 5), true)
			EXPECT( Parser_Test("(1)+(2)", 3), true)
			EXPECT( Parser_Test("(1+2)*3", 9), true)
			EXPECT( Parser_Test("2*(5+3)", 16), true)
		}TEST_END

		TEST_BEGIN("Unary operators") {
		EXPECT(Parser_Test("-1*20", -20), true)
			EXPECT(Parser_Test("-(1+4)", -5), true)
			EXPECT(Parser_Test("(-1)+(-2)", -3), true)
			EXPECT(Parser_Test("-5*3", -15), true)
			EXPECT(Parser_Test("2-(5+3)", -6), true)
		}TEST_END

		TEST_BEGIN("Complex parenthesis"){

			EXPECT(Parser_Test("2+(5*3)",
			                    2+(5*3)),
								true)

			EXPECT(Parser_Test("2*(5+3)+2",
			                    2*(5+3)+2
								), true)

			EXPECT(Parser_Test("(2-(5+3))-2+(1+1)",
			                    (2-(5+3))-2+(1+1)
								), true)

			EXPECT(Parser_Test("(2 -(5+3 )-2)+9/(1- 0.54)",
			                    (2 -(5+3 )-2)+9/(1- 0.54)
								), true)

			EXPECT(Parser_Test("1/3"
			                   , 1.0F/3.0F
							   ), true)
		}TEST_END		

		TEST_BEGIN("Function call"){
			EXPECT(Parser_Test("returnNumber(5)", 5), true)
			EXPECT(Parser_Test("returnNumber(1)", 1), true)
			EXPECT(Parser_Test("sqrt(81)", 9), true)
			EXPECT(Parser_Test("pow(2,2)", 4), true)
		}TEST_END

		TEST_BEGIN("Function-like operators call"){
			EXPECT(Parser_Test("operator*(2,2)", double(4)), true)
			EXPECT(Parser_Test("operator>(2,2)", false), true)
			EXPECT(Parser_Test("operator-(3,2)", double(1)), true)
			EXPECT(Parser_Test("operator+(2,2)", double(4)), true)
			EXPECT(Parser_Test("operator/(4,2)", double(2)), true)
		}TEST_END

		TEST_BEGIN("Imbricated functions") {
			EXPECT(Parser_Test("returnNumber(5+3)", 8), true)
			EXPECT(Parser_Test("returnNumber(returnNumber(1))", 1), true)
			EXPECT(Parser_Test("returnNumber(returnNumber(1) + returnNumber(1))", 2), true)
		}TEST_END

		TEST_BEGIN("Successive assigns") {
			EXPECT(Parser_Test("a = b = 5", 5), true)
			EXPECT(Parser_Test("a = b = c = 10", 10), true)
		}TEST_END

		TEST_BEGIN("Strings tests") {
			EXPECT(Parser_Test("a = \"coucou\"", "coucou"), true)
			EXPECT(Parser_Test("\"hello \" + \"world\"", "hello world"), true)
            EXPECT(Parser_Test("a = string(15)", "15"), true)
            EXPECT(Parser_Test("a = string(-15)", "-15"), true)
            EXPECT(Parser_Test("a = string(-15.5)", "-15.5"), true)
            EXPECT(Parser_Test("b = string(true)", "true"), true)
            EXPECT(Parser_Test("b = string(false)", "false"), true)
        }TEST_END

	}TEST_END

	return s_lastGroupTestPassed;
}

bool Node_Tests() {

	TEST_BEGIN("Node"){

		TEST_BEGIN("Node (add member Double)"){
			auto node = std::make_shared<Node>();
			node->add("val");
			node->set("val", double(100));

			auto val = node->get("val");

			EXPECT((double)*val      , double(100))
			EXPECT((std::string)*val , std::to_string(100))
			EXPECT((bool)*val        , true)

		}TEST_END

	}TEST_END

	return s_lastGroupTestPassed;
}

bool WireAndNode_Tests() {

	TEST_BEGIN("Wire and Node"){

		TEST_BEGIN("Connect two nodes with a wire"){

			auto a = std::make_shared<Node>();
			a->add("output");

			auto b = std::make_shared<Node>();
			b->add("input");

			auto wire = Node::Connect(a->get("output"), b->get("input"));

			EXPECT(wire->getSource() , a->get("output"))
			EXPECT(wire->getTarget() , b->get("input"))

			Node::Disconnect(wire);

		}TEST_END

		TEST_BEGIN("Disconnect a wire"){

			auto a = std::make_shared<Node>();
			a->add("output");

			auto b = std::make_shared<Node>();
			b->add("input");

			auto wire = Node::Connect(a->get("output"), b->get("input"));

			Node::Disconnect(wire);

			EXPECT(a->getOutputWireCount(), 0);
			EXPECT(b->getInputWireCount(), 0);
		}TEST_END

	}TEST_END

	return s_lastGroupTestPassed;
}

bool Test_RunAll()
{
	GLOBAL_TEST_BEGIN{

		TEST_BEGIN("Member"){
			EXPECT( Member_Connections_Tests(), true)
			EXPECT( Member_AsBoolean_Tests(), true)
			EXPECT( Member_AsString_Tests(), true)
			EXPECT( Member_AsNumber_Tests(), true)
		}TEST_END

		TEST_BEGIN("Parser"){
			EXPECT( Parser_Tests(), true)
		}TEST_END

		TEST_BEGIN("Node"){
			EXPECT( Node_Tests(), true)
		}TEST_END

		TEST_BEGIN("Wire and Node"){
			EXPECT( WireAndNode_Tests(), true)
		}TEST_END

		TEST_BEGIN("Biology (DNA to Protein)") {
			EXPECT( Parser_Test("DNAtoProtein(\"TAA\")", "_"), true)
			EXPECT( Parser_Test("DNAtoProtein(\"TAG\")", "_"), true)
			EXPECT( Parser_Test("DNAtoProtein(\"TGA\")", "_"), true)
			EXPECT( Parser_Test("DNAtoProtein(\"ATG\")", "M"), true)
		}TEST_END
	}

	GLOBAL_TEST_END

	return GLOBAL_TEST_RESULT;
}
