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

#include "defs.h"
#include <algorithm>
#include <string>

bool         wstrFromUtf8(const std::string &s, std::wstring *out);
std::wstring wstrFromAnsi(const std::string &s);
std::string  ansiFromwstr(const std::wstring &s);
std::string utf8FromWstr(const std::wstring &s);
std::string extractFileNameWithoutExtension(const std::string &s);

uint64_t msecTime();
void     debugBreak();
void     MessageBoxCall();

#define CONSOLE_GREEN 10
#define CONSOLE_RED 12
#define CONSOLE_GRAY 8
void consoleSetColor(int col);

template <typename Ch>
std::basic_string<Ch> toLower(const std::basic_string<Ch> &s) {
	std::basic_string<Ch> c(s);
	std::transform(c.begin(), c.end(), c.begin(), [](Ch c) {
		return std::tolower(c);
	});
	return c;
}

template <typename Ch, typename F>
void trim(std::basic_string<Ch> &str, const F &pred) {
	size_t first = 0;
	size_t len   = str.length();
	if (len == 0)
		return;
	while (first < len && pred(str[first]))
		++first;
	if (first == len) {
		str = std::basic_string<Ch>();
		return;
	}
	auto last = (int)len - 1;
	while (last >= 0 && pred(str[last]))
		--last;
	if (first != 0 || last != str.length() - 1)
		str = str.substr(first, last - first + 1);
}

template <typename Ch>
void trimSpaces(std::basic_string<Ch> &str) {
	trim(str, [](Ch c) -> bool { return c == ' ' || c == '\t' || c == '\n'; });
}

template <typename Ch>
void strip(std::basic_string<Ch> &str, const std::basic_string<Ch> &remove) {
	trim(str, [&](Ch c) -> bool {
		return remove.find(c) != std::basic_string<Ch>::npos;
	});
}
