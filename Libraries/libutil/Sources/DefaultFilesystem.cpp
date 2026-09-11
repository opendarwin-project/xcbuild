/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#include <libutil/DefaultFilesystem.h>
#include <libutil/FSUtil.h>
#include <libutil/Relative.h>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

using libutil::DefaultFilesystem;
using libutil::Filesystem;
using libutil::Permissions;

bool DefaultFilesystem::exists(std::string const &path) const
{
	std::error_code ec;
	return fs::exists(fs::path(path), ec);
}

ext::optional<Filesystem::Type> DefaultFilesystem::type(
    std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::symlink_status(fs::path(path), ec);
	if (ec || !fs::exists(status)) {
		return ext::nullopt;
	}

	if (fs::is_symlink(status)) {
		return Type::SymbolicLink;
	} else if (fs::is_directory(status)) {
		return Type::Directory;
	} else if (fs::is_regular_file(status)) {
		return Type::File;
	} else {
		return ext::nullopt;
	}
}

bool DefaultFilesystem::isReadable(std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::status(fs::path(path), ec);
	if (ec) {
		return false;
	}
	auto perms = status.permissions();
	return (perms & (fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read)) != fs::perms::none;
}

bool DefaultFilesystem::isWritable(std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::status(fs::path(path), ec);
	if (ec) {
		return false;
	}
	auto perms = status.permissions();
	return (perms & (fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write)) != fs::perms::none;
}

bool DefaultFilesystem::isExecutable(std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::status(fs::path(path), ec);
	if (ec) {
		return false;
	}
	auto perms = status.permissions();
	return (perms & (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)) != fs::perms::none;
}

static Permissions ConvertPermissions(fs::perms p)
{
	Permissions permissions;
	permissions.flag(Permissions::Flag::Sticky, (p & fs::perms::sticky_bit) != fs::perms::none);
	permissions.flag(Permissions::Flag::SetUserID, (p & fs::perms::set_uid) != fs::perms::none);
	permissions.flag(Permissions::Flag::SetGroupID, (p & fs::perms::set_gid) != fs::perms::none);

	permissions.user(Permissions::Permission::Read, (p & fs::perms::owner_read) != fs::perms::none);
	permissions.user(Permissions::Permission::Write, (p & fs::perms::owner_write) != fs::perms::none);
	permissions.user(Permissions::Permission::Execute, (p & fs::perms::owner_exec) != fs::perms::none);

	permissions.group(Permissions::Permission::Read, (p & fs::perms::group_read) != fs::perms::none);
	permissions.group(Permissions::Permission::Write, (p & fs::perms::group_write) != fs::perms::none);
	permissions.group(Permissions::Permission::Execute, (p & fs::perms::group_exec) != fs::perms::none);

	permissions.other(Permissions::Permission::Read, (p & fs::perms::others_read) != fs::perms::none);
	permissions.other(Permissions::Permission::Write, (p & fs::perms::others_write) != fs::perms::none);
	permissions.other(Permissions::Permission::Execute, (p & fs::perms::others_exec) != fs::perms::none);

	return permissions;
}

static fs::perms ConvertPermissionsBack(Permissions const &permissions)
{
	fs::perms p = fs::perms::none;
	if (permissions.flag(Permissions::Flag::Sticky)) p |= fs::perms::sticky_bit;
	if (permissions.flag(Permissions::Flag::SetUserID)) p |= fs::perms::set_uid;
	if (permissions.flag(Permissions::Flag::SetGroupID)) p |= fs::perms::set_gid;

	if (permissions.user(Permissions::Permission::Read)) p |= fs::perms::owner_read;
	if (permissions.user(Permissions::Permission::Write)) p |= fs::perms::owner_write;
	if (permissions.user(Permissions::Permission::Execute)) p |= fs::perms::owner_exec;

	if (permissions.group(Permissions::Permission::Read)) p |= fs::perms::group_read;
	if (permissions.group(Permissions::Permission::Write)) p |= fs::perms::group_write;
	if (permissions.group(Permissions::Permission::Execute)) p |= fs::perms::group_exec;

	if (permissions.other(Permissions::Permission::Read)) p |= fs::perms::others_read;
	if (permissions.other(Permissions::Permission::Write)) p |= fs::perms::others_write;
	if (permissions.other(Permissions::Permission::Execute)) p |= fs::perms::others_exec;

	return p;
}

ext::optional<Permissions> DefaultFilesystem::readFilePermissions(
    std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::status(fs::path(path), ec);
	if (ec) {
		return ext::nullopt;
	}
	return ConvertPermissions(status.permissions());
}

ext::optional<Permissions> DefaultFilesystem::readSymbolicLinkPermissions(
    std::string const &path) const
{
	std::error_code ec;
	fs::file_status status = fs::symlink_status(fs::path(path), ec);
	if (ec) {
		return ext::nullopt;
	}
	return ConvertPermissions(status.permissions());
}

ext::optional<Permissions> DefaultFilesystem::readDirectoryPermissions(
    std::string const &path) const
{
	return readFilePermissions(path);
}

bool DefaultFilesystem::writeFilePermissions(std::string const &path,
    Permissions::Operation operation, Permissions permissions)
{
	ext::optional<Permissions> updated = this->readFilePermissions(path);
	if (!updated) {
		return false;
	}

	updated->combine(operation, permissions);
	fs::perms p = ConvertPermissionsBack(*updated);

	std::error_code ec;
	fs::permissions(fs::path(path), p, ec);
	return !ec;
}

bool DefaultFilesystem::writeSymbolicLinkPermissions(std::string const &path,
    Permissions::Operation operation, Permissions permissions)
{
	ext::optional<Permissions> updated = this->readSymbolicLinkPermissions(path);
	if (!updated) {
		return false;
	}

	updated->combine(operation, permissions);
	fs::perms p = ConvertPermissionsBack(*updated);

	std::error_code ec;
	fs::permissions(fs::path(path), p, fs::perm_options::nofollow, ec);
	return !ec;
}

bool DefaultFilesystem::writeDirectoryPermissions(std::string const &path,
    Permissions::Operation operation, Permissions permissions, bool recursive)
{
	if (!writeFilePermissions(path, operation, permissions)) {
		return false;
	}

	if (recursive) {
		std::error_code ec;
		for (auto const &entry : fs::recursive_directory_iterator(fs::path(path), ec)) {
			if (entry.is_symlink()) {
				writeSymbolicLinkPermissions(entry.path().string(), operation, permissions);
			} else {
				writeFilePermissions(entry.path().string(), operation, permissions);
			}
		}
		if (ec) {
			return false;
		}
	}

	return true;
}

bool DefaultFilesystem::createFile(std::string const &path)
{
	if (this->isWritable(path)) {
		return true;
	}

	std::ofstream ofs(fs::path(path), std::ios::out | std::ios::app);
	return ofs.is_open();
}

bool DefaultFilesystem::read(std::vector<uint8_t> *contents,
    std::string const &path, size_t offset, ext::optional<size_t> length) const
{
	std::ifstream ifs(fs::path(path), std::ios::in | std::ios::binary | std::ios::ate);
	if (!ifs.is_open()) {
		return false;
	}

	std::streamsize size = ifs.tellg();
	if (size < 0) {
		return false;
	}

	if (offset > static_cast<size_t>(size)) {
		return false;
	}

	size_t toRead = static_cast<size_t>(size) - offset;
	if (length) {
		if (*length > toRead) {
			return false;
		}
		toRead = *length;
	}

	ifs.seekg(offset, std::ios::beg);
	contents->resize(toRead);
	if (toRead > 0) {
		ifs.read(reinterpret_cast<char *>(contents->data()), toRead);
		if (!ifs) {
			return false;
		}
	}

	return true;
}

bool DefaultFilesystem::write(
    std::vector<uint8_t> const &contents, std::string const &path)
{
	std::ofstream ofs(fs::path(path), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!ofs.is_open()) {
		return false;
	}

	if (!contents.empty()) {
		ofs.write(reinterpret_cast<const char *>(contents.data()), contents.size());
	}

	return ofs.good();
}

bool DefaultFilesystem::copyFile(std::string const &from, std::string const &to)
{
	std::error_code ec;
	fs::copy_file(fs::path(from), fs::path(to), fs::copy_options::overwrite_existing, ec);
	return !ec;
}

bool DefaultFilesystem::removeFile(std::string const &path)
{
	std::error_code ec;
	return fs::remove(fs::path(path), ec);
}

ext::optional<std::string> DefaultFilesystem::readSymbolicLinkCanonical(
    std::string const &path, bool *directory) const
{
	std::error_code ec;
	fs::path p = fs::path(path);
	fs::file_status status = fs::symlink_status(p, ec);
	if (ec || !fs::exists(status)) {
		return ext::nullopt;
	}

	fs::path canonical = fs::canonical(p, ec);
	if (ec) {
		return ext::nullopt;
	}

	if (canonical == p) {
		return ext::nullopt;
	}

	if (directory) {
		*directory = fs::is_directory(canonical, ec);
	}

	return canonical.string();
}

ext::optional<std::string> DefaultFilesystem::readSymbolicLink(
    std::string const &path, bool *directory) const
{
	std::error_code ec;
	fs::path p = fs::path(path);
	fs::file_status status = fs::symlink_status(p, ec);
	if (ec || !fs::is_symlink(status)) {
		return ext::nullopt;
	}

	fs::path target = fs::read_symlink(p, ec);
	if (ec) {
		return ext::nullopt;
	}

	if (directory) {
		*directory = fs::is_directory(p, ec);
	}

	return target.string();
}

bool DefaultFilesystem::writeSymbolicLink(
    std::string const &target, std::string const &path, bool directory)
{
	(void)directory;
	std::error_code ec;
	fs::create_symlink(fs::path(target), fs::path(path), ec);
	return !ec;
}

bool DefaultFilesystem::copySymbolicLink(
    std::string const &from, std::string const &to)
{
	std::error_code ec;
	fs::copy_symlink(fs::path(from), fs::path(to), ec);
	return !ec;
}

bool DefaultFilesystem::removeSymbolicLink(std::string const &path)
{
	return removeFile(path);
}

bool DefaultFilesystem::createDirectory(std::string const &path, bool recursive)
{
	std::error_code ec;
	if (recursive) {
		fs::create_directories(fs::path(path), ec);
	} else {
		fs::create_directory(fs::path(path), ec);
	}
	return !ec;
}

bool DefaultFilesystem::readDirectory(std::string const &path, bool recursive,
    std::function<void(std::string const &)> const &cb) const
{
	std::error_code ec;
	fs::path basePath = fs::path(path);
	if (recursive) {
		for (auto const &entry : fs::recursive_directory_iterator(basePath, ec)) {
			std::error_code relEc;
			fs::path rel = fs::relative(entry.path(), basePath, relEc);
			if (!relEc) {
				cb(rel.string());
			}
		}
	} else {
		for (auto const &entry : fs::directory_iterator(basePath, ec)) {
			cb(entry.path().filename().string());
		}
	}
	return !ec;
}

bool DefaultFilesystem::copyDirectory(
    std::string const &from, std::string const &to, bool recursive)
{
	std::error_code ec;
	auto options = fs::copy_options::overwrite_existing;
	if (recursive) {
		options |= fs::copy_options::recursive;
	}
	fs::copy(fs::path(from), fs::path(to), options, ec);
	return !ec;
}

bool DefaultFilesystem::removeDirectory(std::string const &path, bool recursive)
{
	std::error_code ec;
	if (recursive) {
		fs::remove_all(fs::path(path), ec);
	} else {
		fs::remove(fs::path(path), ec);
	}
	return !ec;
}

std::string DefaultFilesystem::resolvePath(std::string const &path) const
{
	std::error_code ec;
	fs::path resolved = fs::canonical(fs::path(path), ec);
	if (!ec) {
		return resolved.string();
	}
	resolved = fs::weakly_canonical(fs::path(path), ec);
	if (!ec) {
		return resolved.string();
	}
	return path;
}
