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
#include "myTest.h"

#include "PyVM/ObjPool.h"
#include "PyVM/objects.h"
#include "PyVM/BufferAccess.h"

#include <iostream>
#include <fstream>

#define EXPECT_NO_THROW_PYS( call ) try { call; } \
	catch(const PyException& e) { std::cout << "*** EXCEPTION: " << e.trackback << "\n" << e.what() << std::endl;  FAIL(); } \
	catch(...) { std::cout << "unknown exception" << std::endl; FAIL(); }



class TestObject {
public:
	TestObject() = default;
    TestObject(int x) : m_x(x) {}
    RefCount<TestObject> count;
	int m_x = 0;
};


TEST(PyVM, pool_object_removal)
{
    ObjPool<TestObject> pool;
    PoolPtr<TestObject> a = pool.add(new TestObject(1));
    PoolPtr<TestObject> b = pool.add(new TestObject(2));
    PoolPtr<TestObject> c = pool.add(new TestObject(3));

    b.reset();
    ASSERT_EQ(pool.size(), 2); 
    a.reset();
    ASSERT_EQ(pool.size(), 1);
    c.reset();
    ASSERT_EQ(pool.size(), 0);
}

TEST(PyVM, stack_operations) {

    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    ASSERT_EQ(s.peek(0), 3);
    ASSERT_EQ(s.peek(1), 2);
    ASSERT_EQ(s.peek(2), 1);

    ASSERT_THROW(s.peek(3), PyException);

    ASSERT_THAT(s.data(), ElementsAre(1,2,3));
    s.pushAt(1, 4);
    ASSERT_THAT(s.data(), ElementsAre(1,2,4,3));
    s.pushAt(4, 5);
    ASSERT_THAT(s.data(), ElementsAre(5,1,2,4,3));
    ASSERT_THROW(s.pushAt(6, 66), PyException);
}



class PyVMTest : public Test 
{
public:
    // called before the first
    static void SetUp() {
        vm.reset(new PyVM);
        vm->setStdout(&std::cout);

        s_path = "./";
        
        vm->importPycFile(s_path + "imped_module.pyc");
        mod = vm->importPycFile(s_path + "test_module.pyc");
        ASSERT_FALSE(mod.isNull());

       // s_interp.init();
    }
    // called after the last
    static void TearDown() {
        mod.reset(); // global variable holds an internal reference in the vm
        vm.reset(); 
    }

    static std::shared_ptr<PyVM> vm;
    static ModuleObjRef mod;
	static std::string s_path;
   // static Interpreter s_interp;  // CPython interpreter initialized for bridge testing
};

std::shared_ptr<PyVM> PyVMTest::vm;
ModuleObjRef PyVMTest::mod;
std::string PyVMTest::s_path;
//Interpreter PyVMTest::s_interp;
//Interpreter* g_interp = &PyVMTest::s_interp;

int ctest0() {
	std::cout << "C0" << std::endl;
    return 1;
}

// 3 arguments with return value
bool ctest1(int a, bool b, const std::string& c) {
	std::cout << "C a=" << a << " b=" << b << " c=" << c << std::endl;
    return false;
}

void ctest2(int a, const std::string& c) {
	std::cout << "C a=" << a << " c=" << c << std::endl;
}

std::string ctest3(int a, std::string b) {
	std::cout << "C a=" << a << " b=" << b << std::endl;
    return "XX";
}

void ctest4(float f, const std::wstring& wa) {
	std::cout << "C f=" << f << " wa=" << ansiFromwstr(wa) << std::endl;
}

void ctest5(const std::vector<ObjRef>& v) {
	std::cout << "C v5=" << v.size() << std::endl;
}

void ctest6(const std::vector<int>& v) {
	std::cout << "C v6=" << v.size() << std::endl;
}


TEST_F(PyVMTest, calling_c_func) {
    ASSERT_FALSE(mod.isNull());
    try {
        mod->def("ctest0", ctest0);
        mod->def("ctest1", ctest1);
        mod->def("ctest2", ctest2);
        mod->def("ctest3", ctest3);
        mod->def("ctest4", ctest4);
        mod->def("ctest5", ctest5);
        mod->def("ctest6", ctest6);
        EXPECT_NO_THROW( vm->call("test_module.testCfuncs") );
    }
    catch(...) { FAIL(); }
}



class TestCClass {
public:
	TestCClass(const std::string& id) :m_id(id) {}
	void cmethod1(const std::string& a) {
		std::cout << "Method1 this=" << (size_t)this << " " << m_id << " a=" << a << std::endl;
    }

    int cmethod2() {
		std::cout << "Method2 this=" << (size_t)this << " " << m_id << std::endl;
        return 2;
    }

	bool cmethod3(int a, bool b, const std::string& c) {
		std::cout << "Method3 a=" << a << " b=" << b << " c=" << c << " " << m_id << std::endl;
        return false;
    }

	int cmethod4(int a, bool b) {
		std::cout << "Method4 a=" << a << " b=" << b << " " << m_id << std::endl;
        return 42;
    }

	void cmethod5(int a, std::string b, bool c) {
		std::cout << "Method5 a=" << a << " b=" << b << " c=" << c << " " << m_id << std::endl;
    }

	std::string m_id;
};

TEST_F(PyVMTest, calling_cpp_method) {
    try {
        auto cls = mod->class_<TestCClass>("TestCClass");
        cls->def(&TestCClass::cmethod1, "cmethod1");
        cls->def(&TestCClass::cmethod2, "cmethod2");
        cls->def(&TestCClass::cmethod3, "cmethod3");
        cls->def(&TestCClass::cmethod4, "cmethod4");
        cls->def(&TestCClass::cmethod5, "cmethod5");
        TestCClass ci("me");
        InstanceObjRef ins = cls->instancePtr(&ci);
        EXPECT_NO_THROW(vm->call("test_module.testCMethod", ins));

        //attr of None vm->call("test_module.testCMethod", cls->instancePtr((TestCClass*)NULL));
    }
    catch(...) { FAIL(); }
}


TEST_F(PyVMTest, builtin_types_and_funcs) 
{
    EXPECT_NO_THROW_PYS( vm->call("test_module.testf", 3, 1) );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testIntMathOps") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrMathOps") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testLogicOps") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testFloatMath") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testIntFloatComprasions") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testIntFloatBasicOperations") );    
    EXPECT_NO_THROW_PYS( vm->call("test_module.testIs") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testList") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testCircular") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testFor") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testBuiltInFuncs") );

    EXPECT_NO_THROW_PYS( vm->call("test_module.testSplit") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testUnicode") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testBitOp") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testKeyWordArgs") );

    EXPECT_NO_THROW_PYS( vm->call("test_module.testClass") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testJoin") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testImport") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testListCompr") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testUnpack") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.strIter") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testGetAttr") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrInOp") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testTuple") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testListInOp") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrDictInOp") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrDictSubScript") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrDictValuesFunc"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrDictSize"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrip"));
    
    EXPECT_NO_THROW_PYS( vm->call("test_module.testEq") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testTuple") );

    EXPECT_NO_THROW_PYS( vm->call("test_module.testGen") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testGetAttr") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testLogger") );
    EXPECT_NO_THROW_PYS( vm->call("test_module.testRound"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testStringComparisons") );

    EXPECT_NO_THROW_PYS( vm->call("test_module.testIntCast"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testGlobalInClass"));

    EXPECT_NO_THROW_PYS( vm->call("test_module.testDictCollision"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testXrange"));
}


ObjRef cfunc_kwa(CallArgs& d, PyVM* vm) {
    for(auto it = d.kw.begin(); it != d.kw.end(); ++it) {
		std::cout << it->first << " = " << stdstr(it->second, false) << std::endl;
    }
    return vm->makeNone();
}

TEST_F(PyVMTest, call_c_kw) {
    try {
        mod->def("cfunc_kwa", cfunc_kwa);
        vm->call("test_module.testCKeyWordArgs");
    } catch(...) { FAIL(); }
}

struct VarArgClass {
	std::string varArgFunc(const std::vector<ObjRef>& args) {
		std::stringstream ss;
		std::cout << args.size() << " ";
        ss << args.size() << " ";
        for(auto it = args.begin(); it != args.end(); ++it) {
			std::cout << stdstr(*it) << " ";
            ss << stdstr(*it) << " ";
        }
		std::cout << std::endl;
        return ss.str();
    }
};

TEST_F(PyVMTest, call_var_arg_method) {
    try {
        // variable arguments in method
        VarArgClass i;
        auto cls = mod->class_<VarArgClass>("VarArgClass");
        cls->def(&VarArgClass::varArgFunc, "varArgFunc");
        vm->call("test_module.testVarArgFunc", cls->instancePtr(&i));
    }
    catch(...) { FAIL(); }
}



TEST_F(PyVMTest, callFuncWithVarPosArgs_calledSuccessfully) {
    vm->call("test_module.withVarPos1", 1, 2, "Bla");
    vm->call("test_module.withVarPos2", 1, 2, "Bla");
    vm->call("test_module.withVarPos3");
    vm->call("test_module.testVarPos");
}

struct SomeClass {
	SomeClass(const std::string& _x) : x(_x) {}
	std::string x;
};

void external_method(SomeClass* self, int arg1, const std::string& arg2) {
	std::cout << "in external_method " << self->x << " " << arg1 << " " << arg2 << std::endl;
}

TEST_F(PyVMTest, method_as_c_function) {
    try {
        SomeClass sc("XXX");
        auto cls = mod->class_<SomeClass>("SomeClass");
        cls->def(external_method, "external_method");
        vm->call(cls->instancePtr(&sc)->attr("external_method"), 3, "AAA");
    }
    catch (...) { FAIL(); }
}



TEST_F(PyVMTest, raise_exception) {
    try {
        vm->call("test_module.testException");
    }
    catch(const PyRaisedException& e) {
        EXPECT_EQ(stdstr(e.inst), "Blaa");
    }
}



TEST_F(PyVMTest, numbers_conversion)
{
    int HKLM_SIGNED_VAL = 0x80000002; // normal HKEY_LOCAL_MACHINE is unsigned so it doesn't extend well in 32 bit compile

    ObjRef n;
    n = vm->call("test_module.checkNumber", 1, 0); //HKEY_LOCAL_MACHINE
    EXPECT_EQ(extract<int>(n), (int)0x80000002);

	EXPECT_EQ(extract<int64_t>(n), (int64_t)HKLM_SIGNED_VAL);

    EXPECT_EQ(extract<uint>(n), (uint)0x80000002);
	int64_t c = extract<uint64_t>(n), d = (uint64_t)0x80000002;

	uint64_t a = extract<uint64_t>(n);
	uint64_t b = (uint64_t)HKLM_SIGNED_VAL;

	EXPECT_EQ(extract<uint64_t>(n), (uint64_t)HKLM_SIGNED_VAL);

    n = vm->call("test_module.checkNumber", 2, 0); 
	EXPECT_EQ(extract<uint64_t>(n), 0x100000000);

    n = vm->call("test_module.checkNumber", 3, 0); 
	EXPECT_EQ(extract<uint64_t>(n), 0x7F00000000000000);

    n = vm->call("test_module.checkNumber", 4, 0); 
	EXPECT_EQ(extract<uint64_t>(n), 0xAF00000000000000);

    EXPECT_NO_THROW( vm->call("test_module.checkNumber", 11, (int)HKLM_SIGNED_VAL) ); 
	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 11, (int64_t)HKLM_SIGNED_VAL) );

	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 12, (int64_t)0x100000000) );
	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 12, (uint64_t)0x100000000) );

	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 13, (int64_t)0x7F00000000000000) );
	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 13, (uint64_t)0x7F00000000000000) );

	EXPECT_NO_THROW( vm->call("test_module.checkNumber", 14, (uint64_t)0xAF00000000000000) );


}

static size_t g_got_v = 0;
void charArrCall(size_t v) {
    // check array
    g_got_v = v;
}

template<typename T>
T extractInt(const std::string& s, int start) {
    CHECK(s.size() >= start + sizeof(T), "Unexpected string offset");
    return *(T*)(s.data() + start);
}


TEST_F(PyVMTest, AccessBuffer_usecases) {
    try {
		char *envp[] = {"HellO", "CrueL", "WorlD", nullptr };

        vm->addBuiltin(AccessBuffer::addToModule(vm->mainModule()));
        vm->addBuiltin(BufferBuilder::addToModule(vm->mainModule()));
        vm->addBuiltin("charArrCall", vm->mainModule()->def("charArrCall", charArrCall));
        vm->call("test_module.parseCharArray", (size_t)envp); // calls charArrCall
		EXPECT_EQ((char**)g_got_v, envp); // pointer received by the callback should be the same as envp

		std::string buf;
        buf.resize(15);
        EXPECT_NO_THROW( vm->call("test_module.testAccessBuf", buf) ); 

        ObjRef a(vm->alloc(new StrObject("aaa"))), b(vm->alloc(new StrObject("bbb"))), c(vm->alloc(new StrObject("ccc")));
        ObjRef r = vm->call("test_module.testBuildBuf", a, b, c);
		std::string rs = extract<std::string>(r);
		EXPECT_EQ(rs.size(), 3*sizeof(void*) + sizeof(uint64_t) + sizeof(uint) + sizeof(char)); // should be an array of pointers to the strings
        char** rsp = (char**)rs.data();
        EXPECT_EQ(rsp[0], ((StrObjRef&)a)->v.data());
        EXPECT_EQ(rsp[1], ((StrObjRef&)b)->v.data());
        EXPECT_EQ(rsp[2], ((StrObjRef&)c)->v.data());
		EXPECT_EQ(88888, extractInt<uint64_t>(rs, 3*sizeof(void*) ));
		EXPECT_EQ(666,   extractInt<uint>(rs, 3*sizeof(void*) + sizeof(uint64_t)));
		EXPECT_EQ(111,   extractInt<char>(rs, 3*sizeof(void*) + sizeof(uint64_t) + sizeof(uint)));

        int argc = 4;
        char* argv[4] = { "HellO", "WORLD", "TeST", "this" };
        char* origargv[4] = { argv[0], argv[1], argv[2], argv[3] };
        vm->call("test_module.testArgcArgv", argc, (size_t)argv);
        EXPECT_STREQ("HellO", origargv[0]); // it didn't change the content
        EXPECT_STREQ("hello", argv[0]); // it changed the pointer
        EXPECT_STREQ("WORLD", origargv[1]); 
        EXPECT_STREQ("world", argv[1]); 
        EXPECT_STREQ("TeST", origargv[2]); 
        EXPECT_STREQ("test", argv[2]); 
        EXPECT_STREQ("this", origargv[3]); 
        EXPECT_STREQ("this", argv[3]); 
    }
    catch(const PyException& e) {
        FAIL();
		std::cout << "EXCEPTION: " << e.what() << std::endl << e.trackback << std::endl;
    }
}



TEST_F(PyVMTest, StateClearer_saves_state) {
    int refs = vm->objPool().countRefs();
    int objCount = vm->objPool().size();
    {
        StateClearer sc(vm.get());
        vm->call("test_module.testMem", false);
    }
	std::cout << "BEFORE " << refs << " " << objCount << " AFTER " << vm->objPool().countRefs() << " " << vm->objPool().size() << std::endl;

    EXPECT_EQ(vm->objPool().countRefs(), refs);
    EXPECT_EQ(vm->objPool().size(), objCount);
}

TEST_F(PyVMTest, StateClearer_on_leak) {
    int refs = vm->objPool().countRefs();
    int objCount = vm->objPool().size();
    {
        StateClearer sc(vm.get());
        vm->call("test_module.testMem", true); // leak a reference to a global variable
    }
	std::cout << "BEFORE " << refs << " " << objCount << " AFTER " << vm->objPool().countRefs() << " " << vm->objPool().size() << std::endl;

//     stringstream ss;
//     vm->memDump(ss);
//     FSUtils::saveFile("c:/temp/pyvm/withleak.txt", ss.str());

    // reference count will be the same since we remove a ref from None and add it to the string
    // object count will also be the same since the string existed in the code object
    EXPECT_EQ(vm->objPool().size(), objCount);

}

class CClass {
public:
    CClass(PyVM*)    {ctorWasCalled = true;	}
    ~CClass() {dtorWasCalled = true;}
    int getSomething(){return 5;}

    static bool dtorWasCalled;
    static bool ctorWasCalled;	
};

bool CClass::dtorWasCalled = false;
bool CClass::ctorWasCalled = false;

TEST_F(PyVMTest, CInstanceInitFromPy_NoLeak){		
	{
		auto classObj = mod->class_<CClass>("CClass", CtorDef<NoType>());
		classObj->def(&CClass::getSomething, "getSomething");		
		vm->call("test_module.instanceCObject");
	}
	ASSERT_TRUE(CClass::ctorWasCalled);
	ASSERT_TRUE(CClass::dtorWasCalled);

}


class CClass2 {
public:
    CClass2(PyVM*)    {ctorWasCalled = true;	}
    ~CClass2() {dtorWasCalled = true;}
    int getSomething(){return 5;}

    static bool dtorWasCalled;
    static bool ctorWasCalled;	
};

bool CClass2::dtorWasCalled = false;
bool CClass2::ctorWasCalled = false;

TEST_F(PyVMTest, CInstanceInitFromCUsedByPy_NoLeak){		
    {      
        auto classObj = mod->class_<CClass2>("CClass2", CtorDef<NoType>());
        classObj->def(&CClass2::getSomething, "getSomething");		
		auto cObj = std::make_shared<CClass2>(vm.get());
        auto ref = classObj->instanceSharedPtr(cObj);
        vm->call("test_module.useCObject",ref);
    }
    
    ASSERT_TRUE(CClass2::ctorWasCalled);
    ASSERT_TRUE(CClass2::dtorWasCalled);

}

class CClass3 {
public:
	CClass3(PyVM*, const std::string& id) :m_id(id) { }
	~CClass3() = default;

	std::string m_id;
};


std::string gettingInstance(CClass3* inst) {
	std::cout << "got instance " << inst->m_id << std::endl;
    return inst->m_id;
}

TEST_F(PyVMTest, CInstanceInitFromPyPassedAsArgument_callsFunc) {
	auto classObj = mod->class_<CClass3>("CClass3", CtorDef<std::string>());
    mod->def("gettingInstance", gettingInstance);
    vm->call("test_module.callWithInstance");
}


TEST_F(PyVMTest, testMap){	
    EXPECT_NO_THROW_PYS( vm->call("test_module.testMap") );
}

TEST_F(PyVMTest, testMapItter){	
    EXPECT_NO_THROW_PYS( vm->call("test_module.testMapItter") );
}

TEST_F(PyVMTest, testStrMapItter){	
    StrDictObject* objDict = new StrDictObject;
    auto dictObjRef =  vm->alloc(objDict);
	objDict->v.insert(std::pair<std::string,ObjRef>("a",vm->makeFromT((int)42) ));
	objDict->v.insert(std::pair<std::string,ObjRef>("c",vm->makeFromT((int)43) ));

    EXPECT_NO_THROW_PYS( vm->call("test_module.testStrMapItter",dictObjRef) );
}


TEST_F(PyVMTest, extractOfDouble_intNumber_floatReturned){	
        
    double expected =32.0;
    ObjRef res =  vm->call("test_module.r",vm->makeFromT((int)expected));
    double val = extract<double>(res);
    ASSERT_EQ(val,expected);
}

TEST_F(PyVMTest, extractOfDouble_floatNumber_floatReturned){	

    double expected =32.0;
    ObjRef res =  vm->call("test_module.r",vm->makeFromT((double)expected));
    double val = extract<double>(res);
    ASSERT_EQ(val,expected);
}

TEST_F(PyVMTest, class_with_metaclass_calls_metaclass) {
    EXPECT_NO_THROW_PYS( vm->call("test_module.testMetaClass"));
    
}

TEST_F(PyVMTest, unbounded_method_can_be_called_only_with_instance_of_same_class) {
    EXPECT_NO_THROW_PYS( vm->call("test_module.testUnboundedMethod1"));
    EXPECT_NO_THROW_PYS( vm->call("test_module.testUnboundedMethod2"));
}

template<typename T>
std::function<T> make_function(T *t) {
  return { t };
}

TEST_F(PyVMTest, c_func_lambda) {
    int gota = 0;
	std::string gotb;
	std::function<void(int, const std::string&)> f = [&](int a, const std::string& b) {
        gota = a;
        gotb = b;
    };
    mod->def("ltest", f);
    EXPECT_NO_THROW_PYS( vm->call("test_module.testClambda"));
}

TEST_F(PyVMTest, string_slice) {
    EXPECT_NO_THROW_PYS( vm->call("test_module.testSubstr"));
}



TEST_F(PyVMTest, import_callback) {
	vm->setImportCallback([](const std::string& name) {

		auto p = std::make_unique<std::ifstream>("./" + name + ".pyc", std::ios::binary);

        CHECK(p->good(), "Failed opening module " << name);
		return std::make_pair(std::move(p), true);
    });
    EXPECT_NO_THROW_PYS( vm->call("test_module.testImportCallback"));
}




void testMemLeak(PyVM* vm) {
    for(int i = 0; i < 100; ++i) {
        {
            StateClearer sc(vm);
			std::cout << "refs=" << vm->objPool().countRefs() << " obj=" << vm->objPool().size() << std::endl;
            vm->call("test_module.testMem");
			std::cout << "refs=" << vm->objPool().countRefs() << " obj=" << vm->objPool().size() << std::endl;
        }

		std::cout << "refs=" << vm->objPool().countRefs() << " obj=" << vm->objPool().size() << std::endl;
    }

}

void testPyPolicy() {
    
    //testMemLeak()
    //testInteractive();
}






