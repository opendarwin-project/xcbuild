/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#include <acdriver/Compile/Output.h>
#include <acdriver/Options.h>
#include <acdriver/Result.h>
#include <acdriver/Version.h>
#include <dependency/BinaryDependencyInfo.h>
#include <dependency/DependencyInfo.h>
#include <libutil/Filesystem.h>
#include <plist/Format/Format.h>
#include <plist/Format/XML.h>
#include <xcassets/Asset/Asset.h>

using acdriver::Options;
using acdriver::Result;
using acdriver::Version;
using acdriver::Compile::Output;
using libutil::Filesystem;

Output::Output(std::string const &root, Format format,
    ext::optional<std::string> const &appIcon,
    ext::optional<std::string> const &launchImage,
    NonStandard::ImageTypeSet const &allowedNonStandardImageTypes)
    : _root(root)
    , _format(format)
    , _appIcon(appIcon)
    , _launchImage(launchImage)
    , _allowedNonStandardImageTypes(allowedNonStandardImageTypes)
    , _additionalInfo(plist::Dictionary::New())
{
}

std::string Output::AssetReference(xcassets::Asset::Asset const *asset)
{
	// TODO: include [] for each key
	return asset->path();
}
