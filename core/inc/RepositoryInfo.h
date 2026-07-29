#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QMetaType>
#include <QVector>

namespace CLC
{
    // One executed test executable within a repo's unittest run. suiteName is the
    // unittests/<suite> folder name; exeName is the matched build/Release/*.exe
    // basename (empty when no exe matched). result uses RepositoryJobQueue codes.
    struct UnitTestSuiteResult
    {
        QString suiteName;
        QString exeName;
        int     result = 0;   // 0=success, 1=failed, 2=canceled (matches RepositoryJobQueue)
        QString log;
    };

    struct RepositoryInfo
    {
        enum class BuildStatus { NotBuilt, Building, Error, Success, Canceled };
        enum class TestStatus  { NotRun, Running, Passed, Failed, Error, Canceled };

        QString path;                        // absolute project root
        QString name;                        // folder name
        bool    groupEnabled = true;
        bool    pathExists = true;
        bool    isGitRepo = true;
        bool    hasRemote = false;           // git repo with at least one configured remote

        bool    hasUncommittedChanges = false;
        QString headCommitSubject;

        QString libraryVersion;              // e.g. "1.2.3"; "?" if unknown
        QString templateVersion;             // from "## Template version:"; "?" if unknown
        QString templateVersionBeforeUpdate; // set after a template update this session

        BuildStatus buildStatus = BuildStatus::NotBuilt;
        TestStatus  testStatus  = TestStatus::NotRun;
        QString unitTestLog;                 // combined captured output of last test run
        QVector<UnitTestSuiteResult> unitTestSuites;  // per-suite results of last test run
        QString buildLog;                    // captured output of last build run
    };
}
Q_DECLARE_METATYPE(CLC::RepositoryInfo)
Q_DECLARE_METATYPE(CLC::UnitTestSuiteResult)
Q_DECLARE_METATYPE(QVector<CLC::UnitTestSuiteResult>)
