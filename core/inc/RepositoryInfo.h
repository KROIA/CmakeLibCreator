#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QMetaType>

namespace CLC
{
    struct RepositoryInfo
    {
        enum class BuildStatus { NotBuilt, Building, Error, Success, Canceled };
        enum class TestStatus  { NotRun, Running, Passed, Failed, Error, Canceled };

        QString path;                        // absolute project root
        QString name;                        // folder name
        bool    groupEnabled = true;
        bool    pathExists = true;
        bool    isGitRepo = true;

        bool    hasUncommittedChanges = false;
        QString headCommitSubject;

        QString libraryVersion;              // e.g. "1.2.3"; "?" if unknown
        QString templateVersion;             // from "## Template version:"; "?" if unknown
        QString templateVersionBeforeUpdate; // set after a template update this session

        BuildStatus buildStatus = BuildStatus::NotBuilt;
        TestStatus  testStatus  = TestStatus::NotRun;
        QString unitTestLog;                 // combined captured output of last test run
        QString buildLog;                    // captured output of last build run
    };
}
Q_DECLARE_METATYPE(CLC::RepositoryInfo)
