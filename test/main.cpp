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

#include "PyVM/PyVM.h"
#include "PyVM/objects.h"
#include "PyVM/PyCompile.h"

#include "myTest.h"

std::vector<Test*> Test::s_allTests;

// main for running tests
int main() {
	runAllTests();
}


#ifdef USE_CPYTHON
// main for interactive interpreter
int imain()
{
    PyVM vm;
    vm.runInteractive();
    return 0;
}

// main for running simple sample
int xmain()
{
    PyVM vm;
    vm.importPycFile("C:\\projects\\PyVM\\test.pyc");

	std::string comp;
    compileTextToPycBuf(R"**(
def pyfunc(a, b):
    return a + b;
print pyfunc(1, 2)    
)**", "test2.py", &comp);

    auto module = vm.importPycBuf(comp);

	std::cout << extract<int>(vm.call("test2.pyfunc", 2, 3)) << std::endl;

    return 0;
}


#endif



