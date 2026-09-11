/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#include <acdriver/Compile/Output.h>
#include <acdriver/Compile/StickerSequence.h>
#include <acdriver/Result.h>
#include <libutil/Filesystem.h>

using acdriver::Result;
using acdriver::Compile::Output;
using acdriver::Compile::StickerSequence;
using libutil::Filesystem;

bool StickerSequence::Compile(
    xcassets::Asset::StickerSequence const *stickerSequence,
    Filesystem *filesystem, Output *compileOutput, Result *result)
{
	result->document(Result::Severity::Warning, stickerSequence->path(),
	    { Output::AssetReference(stickerSequence) }, "Not Implemented",
	    "sticker sequence not yet supported");

	return false;
}
