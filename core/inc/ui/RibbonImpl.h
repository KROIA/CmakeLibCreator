#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RibbonWidget.h"

namespace CLC {
	class RibbonImpl : public RibbonWidget::Ribbon
	{
		Q_OBJECT
	public:
		RibbonImpl(QToolBar* parent = nullptr);
		~RibbonImpl();



		struct TemplateManagementButtons
		{
			RibbonWidget::RibbonButton* openTemplatePath = nullptr;
			RibbonWidget::InformativeToolButton* downloadTemplate = nullptr;
		};

		struct ProjectButtons
		{
			RibbonWidget::InformativeToolButton* openExistingProject = nullptr;
			RibbonWidget::InformativeToolButton* saveExistingProject = nullptr;
			RibbonWidget::InformativeToolButton* saveAsNewProject = nullptr;
		};

		struct RepositoryButtons
		{
			// group "General"
			RibbonWidget::InformativeToolButton* openFolders = nullptr;
			RibbonWidget::InformativeToolButton* updateTemplates = nullptr;
			RibbonWidget::InformativeToolButton* refreshStatus = nullptr;
			// group "Git"
			RibbonWidget::InformativeToolButton* pull = nullptr;
			RibbonWidget::InformativeToolButton* push = nullptr;
			RibbonWidget::InformativeToolButton* commit = nullptr;
			RibbonWidget::InformativeToolButton* discard = nullptr;
			// group "Build"
			RibbonWidget::InformativeToolButton* build = nullptr;
			RibbonWidget::InformativeToolButton* clean = nullptr;
			RibbonWidget::InformativeToolButton* unitTest = nullptr;
		};


		static TemplateManagementButtons& getTemplateManagementButtons();
		static ProjectButtons& getProjectButtons();
		static RepositoryButtons& getRepositoryButtons();
	private:
		static RibbonImpl* m_instance;

		TemplateManagementButtons m_templateButtons;
		ProjectButtons m_projectButtons;
		RepositoryButtons m_repositoryButtons;

		RibbonWidget::RibbonTab* m_mainTab;
		RibbonWidget::RibbonTab* m_repoTab;

		RibbonWidget::RibbonButtonGroup* m_templateGroup;
		RibbonWidget::RibbonButtonGroup* m_projectGroup;
		RibbonWidget::RibbonButtonGroup* m_repoGeneralGroup;
		RibbonWidget::RibbonButtonGroup* m_repoGitGroup;
		RibbonWidget::RibbonButtonGroup* m_repoBuildGroup;

	};
}
