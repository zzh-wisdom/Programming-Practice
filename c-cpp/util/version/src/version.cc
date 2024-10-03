#include <cstdlib>

#include <iostream>

#include "version.h"
#include "version.auto_gen.h"

namespace util
{

namespace version
{

const char *GitCommit()
{
    return kGitCommit;
}

const char *GitBranch()
{
    return kGitBranch;
}

const char *GitStatus()
{
    return kGitStatus;
}

const char *BuildTime()
{
    return kBuildTime;
}

const char *BuildUser()
{
    return kBuildUser;
}

const char *BuildHost()
{
    return kBuildHost;
}

std::string VerisonInfo(const std::string &name)
{
    std::string version_info = "\n";
    version_info += std::string("Name:      ") + name + "\n";
    version_info += std::string("GitCommit: ") + GitCommit() + "\n";
    version_info += std::string("GitBranch: ") + GitBranch() + "\n";
    version_info += std::string("GitStatus: ") + GitStatus() + "\n";
    version_info += std::string("BuildTime: ") + BuildTime() + "\n";
    version_info += std::string("BuildUser: ") + BuildUser() + "\n";
    version_info += std::string("BuildHost: ") + BuildHost() + "\n";
    return version_info;
}

}

}
