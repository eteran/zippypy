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

#include <iostream>

enum LogLevel {
	LOGLEVEL_DEBUG,
	LOGLEVEL_ERROR,
};

inline void tostream(std::ostream &) {
}

template <typename T, typename... Args>
inline void tostream(std::ostream &os, T &&v, Args &&... args) {
	os << std::forward<T>(v);
	tostream(os, std::forward<Args>(args)...);
}

template <typename... Args>
void log(LogLevel lvl, Args &&... args) {
	std::ostream &ostr = (lvl == LOGLEVEL_DEBUG) ? std::cout : std::cerr;
	tostream(ostr, std::forward<Args>(args)...);
	ostr << "\n";
}

#define LOG_DEBUG(...)                    \
	do {                                  \
		log(LOGLEVEL_DEBUG, __VA_ARGS__); \
	} while (false)

#define LOG_ERROR(...)                    \
	do {                                  \
		log(LOGLEVEL_ERROR, __VA_ARGS__); \
	} while (false)
