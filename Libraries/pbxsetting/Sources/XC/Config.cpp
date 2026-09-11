/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree.
 */

#include <libutil/Base.h>
#include <libutil/FSUtil.h>
#include <libutil/Filesystem.h>
#include <pbxsetting/XC/Config.h>

using libutil::Filesystem;
using libutil::FSUtil;
using pbxsetting::Environment;
using pbxsetting::Level;
using pbxsetting::Setting;
using pbxsetting::Value;
using pbxsetting::XC::Config;

Config::Entry::Entry(Setting const &setting)
    : _type(Type::Setting)
    , _setting(setting)
{
}

Config::Entry::Entry(Value const &path, std::shared_ptr<Config> const &config)
    : _type(Type::Include)
    , _path(path)
    , _config(config)
{
}

Config::Config(std::string const &path, std::vector<Entry> const &contents)
    : _path(path)
    , _contents(contents)
{
}

Config::~Config() { }

Level Config::level() const
{
	std::vector<Setting> settings;

	for (Entry const &entry : _contents) {
		switch (entry.type()) {
		case Entry::Type::Setting: {
			Setting const &setting = *entry.setting();
			settings.push_back(setting);
			break;
		}
		case Entry::Type::Include: {
			Level level = entry.config()->level();
			settings.insert(settings.end(),
			    level.settings().begin(), level.settings().end());
			break;
		}
		}
	}

	return Level(settings);
}

static ext::optional<Value> ParseInclude(std::string const &value)
{
	std::string path = value;

	/* Strip whitespace. */
	libutil::trim(path);

	/* Verify formatting is correct. */
	if (path.empty() || path.front() != '"' || path.back() != '"') {
		return ext::nullopt;
	}

	/* Remove formatting. */
	path = path.substr(1, path.length() - 2);

	std::string developer = "<DEVELOPER_DIR>";

	if (path.compare(0, developer.size(), developer) == 0) {
		/* Developer-relative path. */
		std::string relative = path.substr(developer.size());
		return Value::Variable("DEVELOPER_DIR") +
		    Value::String(relative);
	} else {
		/* Absolute or relative path. */
		return Value::String(path);
	}
}

static ext::optional<Config::Entry> ParseDirective(Filesystem const *filesystem,
    Environment const &environment, std::string const &directory,
    std::string const &line)
{
	size_t prefixLen = 0;
	if (line.compare(1, 7, "include") == 0) {
		prefixLen = 8;
		if (line.size() > 8 && line[8] == '?') {
			prefixLen = 9;
		}
	}

	if (prefixLen > 0) {
		/* Handle include directive. */
		std::string value = line.substr(prefixLen);
		if (ext::optional<Value> parsed = ParseInclude(value)) {
			/* Determine the path on disk. */
			std::string path = environment.expand(*parsed);
			path = FSUtil::ResolveRelativePath(path, directory);

			/* Load included config. */
			if (ext::optional<Config> config = Config::Load(
				filesystem, environment, path)) {
				return Config::Entry(
				    *parsed, std::make_shared<Config>(*config));
			} else {
				/* Failed to load included config (e.g. missing
				 * Apple internal makefiles) - treat as empty
				 * rather than failing parent config. */
				return Config::Entry(*parsed,
				    std::make_shared<Config>(
					path, std::vector<Config::Entry>()));
			}
		} else {
			/* Failed to parse include. */
			return ext::nullopt;
		}
	} else {
		/* Unknown preprocessor directive. */
		return ext::nullopt;
	}
}

ext::optional<Config> Config::Load(Filesystem const *filesystem,
    Environment const &environment, std::string const &path)
{
	std::string directory = FSUtil::GetDirectoryName(path);

	/* Read in input. */
	std::vector<uint8_t> contents;
	if (!filesystem->read(&contents, path)) {
		return ext::nullopt;
	}

	/* Add trailing newline if missing. */
	if (contents.empty() || contents.back() != '\n') {
		contents.push_back('\n');
	}

	std::vector<Entry> entries;

	bool slash = false;
	bool comment = false;
	std::string line;

	for (uint8_t const &c : contents) {
		if (c == '/') {
			if (slash) {
				/* Two slashes, start comment. */
				if (!comment) {
					line.pop_back();
					comment = true;
				}
			} else {
				/* One slash, prepare for comment. */
				slash = true;
			}
		} else if (slash) {
			/* Just one slash, reset. */
			slash = false;
		}

		if (c == '\r' || c == '\n') {
			/* End comment on newline. */
			comment = false;

			if (!line.empty()) {
				if (line.front() == '#') {
					/* Parse directive. */
					if (ext::optional<Entry> entry =
						ParseDirective(filesystem,
						    environment, directory,
						    line)) {
						entries.push_back(*entry);
					} else {
						/* Failed to parse directive. */
						return ext::nullopt;
					}
				} else {
					/* Trim whitespace. */
					libutil::trim(line);

					if (!line.empty()) {
						/* Remove trailing semicolon. */
						if (line.back() == ';') {
							line.pop_back();
						}

						/* Parse setting value. */
						if (ext::optional<Setting>
							setting =
							    Setting::Parse(
								line)) {
							Entry entry =
							    Config::Entry(
								*setting);
							entries.push_back(
							    entry);
						} else {
							/* Failed to parse
							 * setting. */
							return ext::nullopt;
						}
					}
				}
			}

			/* Start new line. */
			line.clear();
		} else if (!comment) {
			/* Add character. */
			line += c;
		}
	}

	return Config(path, entries);
}
