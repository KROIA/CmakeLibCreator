#include "Logging.h"

namespace CLC
{
	Logging::Logging()
	{
		m_logger = new Log::Logger::ContextLogger("CmakeLibraryCreator");
#ifdef USE_TREE_VIEW
		m_view = new Log::UI::QContextLoggerTreeView(nullptr);
#else
		m_view = new Log::UI::QConsoleView(nullptr);
#endif
	

		m_view->attachLogger(*m_logger);
	}

	Logging::~Logging()
	{
		delete m_view;
		delete m_logger;
	}

	Logging& Logging::getInstance()
	{
		static Logging instance;
		return instance;
	}

	Log::Logger::ContextLogger& Logging::getLogger()
	{
		return *getInstance().m_logger;
	}

#ifdef USE_TREE_VIEW
	Log::UI::QContextLoggerTreeView& Logging::getView()
#else
	Log::UI::QConsoleView& Logging::getView()
#endif
	{
		return *getInstance().m_view;
	}
}