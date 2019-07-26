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

#include "utils.h"
#include <chrono>

#ifdef WIN32

#include <windows.h>

bool wstrFromUtf8(const std::string &s, std::wstring *out) {
	if (s.empty()) {
		out->clear();
		return true;
	}
	int sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), NULL, 0);
	if (sz == 0)
		return false;
	out->resize(sz);
	sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), (LPWSTR)out->data(), sz);
	return sz != 0;
}
std::string utf8FromWstr(const std::wstring &s) {
	if (s.empty()) {
		return string();
	}
	int sz = WideCharToMultiByte(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), NULL, 0, NULL, NULL);
	if (sz == 0)
		return string();
	std::string out;
	out.resize(sz);
	sz = WideCharToMultiByte(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), (LPSTR)out.data(), sz, NULL, NULL);
	return out;
}
#else

bool wstrFromUtf8(const std::string &s, std::wstring *out) {
	return false;
}

std::string utf8FromWstr(const std::wstring &s) {
	std::string s2;
	return s2;
}
#endif

std::wstring wstrFromAnsi(const std::string &s) {
	std::wstring out;
	out.resize(s.size());
	std::copy(s.begin(), s.end(), out.begin());
	return out;
}
std::string ansiFromwstr(const std::wstring &s) {
	std::string out;
	out.reserve(s.size());
	for (wchar_t c : s)
		out.append(1, (char)c); // not correct at all
	return out;
}

#define SLASHES "/\\"

std::string extractFileNameWithoutExtension(const std::string &path) {
	// find the last slash
	size_t startPos = path.find_last_of(SLASHES);
	if (startPos == std::wstring::npos)
		startPos = 0;
	else
		startPos += 1;
	size_t endPos = path.find_last_of('.');
	if (endPos == path.npos || endPos < startPos) {
		endPos = path.length();
	}
	return path.substr(startPos, endPos - startPos);
}

uint64_t msecTime() {
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	return ms.count();
}

#ifdef WIN32

void debugBreak() {
	DebugBreak();
}
void MessageBoxCall() {
	MessageBoxA(nullptr, "Your Message Box", "Hello", MB_OK);
}

void consoleSetColor(int col) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), col);
}

#else

void debugBreak() {
	printf("debugBreak!\n");
	abort();
}

void MessageBoxCall() {
	printf("MessageBox!\n");
}

void consoleSetColor(int col) {
	printf("SetColor!\n");
}
#endif
