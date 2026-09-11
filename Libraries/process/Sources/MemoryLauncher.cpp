/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#include <libutil/Filesystem.h>
#include <process/Context.h>
#include <process/MemoryLauncher.h>

using libutil::Filesystem;
using process::Context;
using process::MemoryLauncher;

MemoryLauncher::MemoryLauncher(
    std::unordered_map<std::string, Handler> const &handlers)
    : Launcher()
    , _handlers(handlers)
{
}

MemoryLauncher::~MemoryLauncher() { }

ext::optional<int> MemoryLauncher::launch(
    Filesystem *filesystem, Context const *context)
{
	auto it = _handlers.find(context->executablePath());
	if (it != _handlers.end()) {
		return it->second(filesystem, context);
	} else {
		return ext::nullopt;
	}
}
