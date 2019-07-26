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

#include "baseObject.h"

#include <vector>
#include <string>


class Deserialize;
class PyVM;

class CodeDefinition 
{
public:
    static ObjRef parsePyc(std::istream& iss, PyVM* vm, bool hasHeader);

    void parseCode(Deserialize& s, PyVM* vm);
public:
    std::string co_name;
    uint co_argcount;
    uint co_nlocals;
    std::vector<std::string> co_varnames;
    std::vector<std::string> co_cellvars;
    std::vector<std::string> co_freevars;
    std::string co_code;
    std::vector<ObjRef> co_consts;
    std::vector<std::string> co_names;
    std::string co_filename;
    uint co_firstlineno;
    std::string co_lnotab;
    uint co_stacksize;
    uint co_flags;
};
