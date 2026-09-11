/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#ifndef __libutil_Base_h
#define __libutil_Base_h

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace libutil {

static inline std::string trim(std::string_view s)
{
	auto is_space = [](unsigned char ch) { return std::isspace(ch); };
	auto trimmed = s | std::views::drop_while(is_space) |
	    std::views::reverse | std::views::drop_while(is_space) |
	    std::views::reverse;

	return std::ranges::to<std::string>(trimmed);
}

template <typename T, typename U>
static inline std::unique_ptr<T> static_unique_pointer_cast(
    std::unique_ptr<U> &&p)
{
	return std::unique_ptr<T>(static_cast<T *>(p.release()));
}

}

#endif // !__libutil_Base_h
