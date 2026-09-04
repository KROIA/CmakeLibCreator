#include <QApplication>
#include <iostream>

#include "UnitTest.h"
#include "UnitTest_Gui.h"
#include "tests.h"

int main(int argc, char* argv[])
{
	// Resources anchors settings.json and data/template under these two names. Without them the
	// test binary would look under its own executable name and every export test would skip.
	QCoreApplication::setOrganizationName("KROIA");
	QCoreApplication::setApplicationName("CmakeLibCreator");

	QApplication application(argc, argv);

	// CI is the default, the opposite of the UnitTest GuiExample: a delay only helps a human.
	bool watch = false;
	for (int index = 1; index < argc; ++index)
		watch = watch || QString::fromLocal8Bit(argv[index]) == "--watch";

	if (watch)
	{
		UnitTest::Gui::setStepDelay(650);
		UnitTest::Gui::setHighlightEnabled(true);
		std::cout << "Running in WATCH mode - the GUI test drives a visible window.\n\n";
	}

	UnitTest::LibraryInfo::printInfo();

	std::cout << "Running " << UnitTest::Test::getTests().size() << " tests...\n";
	UnitTest::Test::TestResults results;
	UnitTest::Test::runAllTests(results);
	UnitTest::Test::printResults(results);

	return results.getSuccess() ? 0 : 1;
}
