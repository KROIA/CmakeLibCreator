#pragma once

#include "CmakeLibraryCreator_base.h"
#include "Logger.h"

//#define USE_TREE_VIEW

namespace CLC
{
	class Logging
	{
		public:
			Logging();
			~Logging();

			static Logging &getInstance();

			static Log::Logger::ContextLogger &getLogger();
#ifdef USE_TREE_VIEW
			static Log::UI::QContextLoggerTreeView& getView();
#else
			static Log::UI::QConsoleView& getView();
#endif

		private:
		Log::Logger::ContextLogger* m_logger;
#ifdef USE_TREE_VIEW
		Log::UI::QContextLoggerTreeView* m_view;
#else
		Log::UI::QConsoleView* m_view;
#endif
	};

}