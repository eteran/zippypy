// Copyright 2015 by Intigua, Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <vector>
#include <string>
#include <exception>
#include <iostream>
#include <sstream>
#include <set>
#include "PyVM/utils.h"

struct TestAssert : public std::exception
{
	TestAssert(const std::string& desc) :m_desc(desc) {}

	const char* what() const noexcept override {
        return m_desc.c_str();
    }

	std::string m_desc;
};

class Case 
{
public:
    virtual void run() = 0;
    virtual const char* name() const = 0;

	void reportFailed(const std::string& str) {
        if (m_failed)
            return;
        m_failstr = str;
        m_failed = true;
    }
    bool m_failed = false;
	std::string m_failstr;
};


typedef void(*TSetupFunc)();


class Test
{
public:
    void regSelf() {
        s_allTests.push_back(this); 
    }

    void runAll() {
        m_failedCount = 0;
        m_passedCount = 0;
        try {
            (*m_setup)();
        }
        catch (const TestAssert& e) {
            consoleSetColor(CONSOLE_RED);
			std::cout << "** FAILED SETUP";
            consoleSetColor(CONSOLE_GRAY);
			std::cout << ":" << e.what() << std::endl;
            ++m_failedCount;
            return;
        }
        for(auto* c: m_cases) {
            try {
                c->run();
            }
            catch(const TestAssert& e) {
                consoleSetColor(CONSOLE_RED);
				std::cout << "** FAILED: ";
                consoleSetColor(CONSOLE_GRAY);
				std::cout << c->name() << ":" << e.what() << std::endl;
                ++m_failedCount;
                continue;
            }
            if (c->m_failed) {
                consoleSetColor(CONSOLE_RED);
				std::cout << "** FAILED: ";
                consoleSetColor(CONSOLE_GRAY);
				std::cout << c->name() << ":" << c->m_failstr << std::endl;
                ++m_failedCount;
            }
            else {
                consoleSetColor(CONSOLE_GREEN);
				std::cout << "** PASSED: ";
                consoleSetColor(CONSOLE_GRAY);
				std::cout << c->name() << std::endl;
                ++m_passedCount;
            }
        }
        try {
            (*m_teardown)();
        }
        catch (const TestAssert& e) {
            consoleSetColor(CONSOLE_RED);
			std::cout << "** FAILED TEARDOWN";
            consoleSetColor(CONSOLE_GRAY);
			std::cout << ":" << e.what() << std::endl;
            ++m_failedCount;
            return;
        }
    }

    TSetupFunc m_setup;
    TSetupFunc m_teardown;
	std::vector<Case*> m_cases;
    int m_failedCount = 0, m_passedCount = 0;

    static void SetUp() {}
    static void TearDown() {}

	static std::vector<Test*> s_allTests;
};



template<typename TestCls>
struct CaseReg {
    CaseReg(Case* cs) {
        static TestCls s_instance;
        static bool s_wasReg = false;
        if (!s_wasReg) {
            s_instance.regSelf();
            s_wasReg = true;
        }
        s_instance.m_setup = &TestCls::SetUp;
        s_instance.m_teardown = &TestCls::TearDown; // happens again and again
        s_instance.m_cases.push_back(cs);
    }
};

#define TEST_COMMON(testName, caseName, regTo) \
struct testName##__##caseName : public Case, public regTo { \
	virtual ~testName##__##caseName() = default; \
    static CaseReg<regTo> g_register; \
    virtual void run(); \
    virtual const char* name() const { return #testName "::" #caseName; } \
}; \
CaseReg<regTo> testName##__##caseName::g_register(new testName##__##caseName); \
void testName##__##caseName::run()

#define TEST_F(testName, caseName) TEST_COMMON(testName, caseName, testName)
#define TEST(testName, caseName) TEST_COMMON(testName, caseName, Test)

inline void runAllTests()
{
	int failed = 0;
	int passed = 0;

	for(auto t: Test::s_allTests) {
        t->runAll();
        failed += t->m_failedCount;
        passed += t->m_passedCount;
    }
    consoleSetColor((failed > 0) ? CONSOLE_RED : CONSOLE_GREEN);

	std::cout << "\n**** failed: " << failed << " passed: " << passed << std::endl;
    consoleSetColor(CONSOLE_GRAY);
}

#define FAIL() do { std::ostringstream ss; ss << __LINE__ << " Failed"; reportFailed(ss.str()); } while(false)

#define TEST_DO_THROW throw TestAssert
#define TEST_DO_REPORT reportFailed

#define COMMON_EQ(a, b, action) if (!((a) == (b))) { std::ostringstream ss; ss << __LINE__ << ": " #a " != " #b " Where\n  first=" << a << "\n  second=" << b; action(ss.str()); }
#define ASSERT_EQ(a, b) COMMON_EQ(a, b, TEST_DO_THROW)
#define EXPECT_EQ(a, b) COMMON_EQ(a, b, TEST_DO_REPORT)

#define COMMON_FALSE(a, action) if (a) { std::ostringstream ss; ss << __LINE__ << ": " #a " is true (need to be false) Where\n  arg=" << a; action(ss.str()); }
#define ASSERT_FALSE(a) COMMON_FALSE(a, TEST_DO_THROW)
#define EXPECT_FALSE(a) COMMON_FALSE(a, TEST_DO_REPORT)

#define COMMON_TRUE(a, action) if (!(a)) { std::ostringstream ss; ss << __LINE__ << ": " #a " is true (need to be false) Where\n  arg=" << a; action(ss.str()); }
#define ASSERT_TRUE(a) COMMON_TRUE(a, TEST_DO_THROW)
#define EXPECT_TRUE(a) COMMON_TRUE(a, TEST_DO_REPORT)

#define EXPECT_STREQ(a, b) EXPECT_EQ(std::string(a), std::string(b))



#define COMMON_THROW(cmd, excp, action) do { \
   bool didThrow = false; \
   std::ostringstream ss; \
   try { cmd; } \
   catch(const excp&) { didThrow = true; } \
   catch(...) { ss << __LINE__ << ": " "Statement `" << #cmd "` Threw something other than " #excp; action(ss.str()); } \
   if (!didThrow) { ss << __LINE__ << ": " "Statement `" << #cmd "` did not throw any exception"; action(ss.str()); } \
} while(false)
#define ASSERT_THROW(cmd, excp) COMMON_THROW(cmd, excp, TEST_DO_THROW)
#define EXPECT_THROW(cmd, excp) COMMON_THROW(cmd, excp, TEST_DO_REPORT)

#define COMMON_NO_THROW(cmd, action) do { \
   std::ostringstream ss; \
   try { cmd; } \
   catch(...) { ss << __LINE__ << ": " "Statement `" << #cmd "` Threw something "; action(ss.str()); } \
} while(false)
#define ASSERT_NO_THROW(cmd) COMMON_NO_THROW(cmd, TEST_DO_THROW)
#define EXPECT_NO_THROW(cmd) COMMON_NO_THROW(cmd, TEST_DO_REPORT)

template<typename T>
struct CElementsAre {
	std::vector<T> elems;
    template<typename TCont>
    bool isTrue(const TCont& cont) {
		std::set<T> checked;
        for(const auto& o: cont) {
            auto it = find(elems.begin(), elems.end(), o);
            if (it == elems.end())
                return false;
            checked.insert(o);
        }
        for(const auto& e: elems) {
            if (checked.count(e) == 0)
                return false;
        }
        return true;
	}

    template<typename TCont>
	std::string disp(const TCont& cont) {
		std::ostringstream ss;
        for(const auto& o: cont)
            ss << o << ", ";
        return ss.str();
    }
};

template<typename T, typename... Args>
CElementsAre<T> ElementsAre(const T& first, const Args&... args) {
	std::vector<T> e = { first, args... };
    return CElementsAre<T> {e};
}
// specialization for string literlas
template<typename... Args>
CElementsAre<std::string> ElementsAre(const char first[], const Args&... args) {
	std::vector<std::string> e = { first, args... };
	return CElementsAre<std::string> {e};
}

#define ASSERT_THAT(argument, conditionCls) do { \
    auto co = conditionCls; \
    if (!(co.isTrue(argument))) { \
		std::ostringstream ss; ss << __LINE__ << ": " "Failed " #conditionCls " for " #argument "\n  which is: " << co.disp(argument); throw TestAssert(ss.str()); } \
    } while(false)
