#pragma once

#include <string>

namespace util
{

namespace version
{

const char *GitCommit();
const char *BuildTime();
const char *BuildUser();
const char *BuildHost();

std::string VerisonInfo(const std::string &name);

}

}
