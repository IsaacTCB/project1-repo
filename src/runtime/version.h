#pragma once

#include <cstdint>
#include <format>
#include <string>

struct VersionInfo {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    const char *config;        /* cmake configuration (Debug|Release|RelDebInfo|MinSizeRel) */
    const char *git_hash;      /* short hash, or "unknown" outside a git checkout */
    bool git_dirty;            /* tracked files had uncommitted changes at build time */
    const char *semver_string; /* "0.1.0" on a clean tagged release, else "0.1.0+abc1234[.dirty]" */

    std::string full_semver_name() const {
        return std::format("pj1-{}-{}", config, semver_string);
    }
};

namespace RuntimeVersion {

const VersionInfo &version();

} /* namespace RuntimeVersion */
