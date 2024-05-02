#include "Logging.h"

namespace CLC
{
	Logging::Logging()
	{
		m_logger = new Log::Logger::ContextLogger("CmakeLibraryCreator");
		m_view = new Log::UI::QContextLoggerTreeView(nullptr);
	

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

	Log::UI::QContextLoggerTreeView& Logging::getView()
	{
		return *getInstance().m_view;
	}
}