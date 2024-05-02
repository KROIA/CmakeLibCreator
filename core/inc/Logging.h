#pragma once

#include "CmakeLibraryCreator_base.h"
#include "Logger.h"


namespace CLC
{
	class Logging
	{
		public:
			Logging();
			~Logging();

			static Logging &getInstance();

			static Log::Logger::ContextLogger &getLogger();
			static Log::UI::QContextLoggerTreeView& getView();

		private:
		Log::Logger::ContextLogger* m_logger;
		Log::UI::QContextLoggerTreeView* m_view;
	};

}