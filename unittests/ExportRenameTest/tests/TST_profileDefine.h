#pragma once

#include "UnitTest.h"
#include "UnitTest_Gui.h"

#include "ui/ProjectSettingsDialog.h"

#include <QLineEdit>
#include <QString>

// LIB_PROFILE_DEFINE used to be derived from the capitals of the library name while
// CMakePresets.json used lib_short_define. "DTI801_Driver" with the short define "DTI801" is
// exactly the disagreement that used to yield DTID_PROFILING and silently disable profiling.
class TST_profileDefine : public UnitTest::Test
{
	TEST_CLASS(TST_profileDefine)
public:
	TST_profileDefine()
		: Test("TST_profileDefine")
	{
		ADD_TEST(TST_profileDefine::profileDefineFollowsTheShortDefine);
		setBreakOnFail(false);
	}

private:

	TEST_FUNCTION(profileDefineFollowsTheShortDefine)
	{
		TEST_START;

		if (!UnitTest::Gui::isAvailable())
		{
			TEST_MESSAGE("No GUI session available - the settings dialog was NOT exercised.");
			return;
		}

		CLC::ProjectSettingsDialog dialog;
		TEST_ASSERT(UnitTest::Gui::showAndWait(&dialog));

		QLineEdit* libraryName = UnitTest::Gui::find<QLineEdit>("libraryName_lineEdit", &dialog);
		QLineEdit* shortDefine = UnitTest::Gui::find<QLineEdit>("libraryNameShort_lineEdit", &dialog);
		QLineEdit* profileDefine = UnitTest::Gui::find<QLineEdit>("libProfileDefine_lineEdit", &dialog);
		TEST_ASSERT_M(libraryName && shortDefine && profileDefine,
			"the settings dialog no longer has the three name fields:\n"
			+ UnitTest::Gui::dumpWidgetTree(&dialog).toStdString());
		if (!libraryName || !shortDefine || !profileDefine)
			return;

		UnitTest::Gui::narrate("Typing a library name - the profile define follows the derived short define");
		TEST_ASSERT(UnitTest::Gui::clearAndType(libraryName, "DTI801_Driver"));
		TEST_ASSERT_M(!shortDefine->text().isEmpty(), "no short define was derived from the library name");
		TEST_COMPARE(profileDefine->text(), shortDefine->text() + "_PROFILING");

		UnitTest::Gui::narrate("Overriding the short define by hand");
		TEST_ASSERT(UnitTest::Gui::clearAndType(shortDefine, "DTI801"));
		TEST_COMPARE(shortDefine->text(), QString("DTI801"));
		TEST_ASSERT_M(profileDefine->text() == QString("DTI801_PROFILING"),
			"the profile define ignored the short define and reads "
			+ profileDefine->text().toStdString()
			+ " - CMakePresets.json would enable a macro the library never checks");
	}
};

TEST_INSTANTIATE(TST_profileDefine);
