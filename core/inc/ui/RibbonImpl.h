#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RibbonWidget.h"

namespace CLC {
	class RibbonImpl : public RibbonWidget::Ribbon
	{
		Q_OBJECT
	public:
		RibbonImpl(QWidget* parent = nullptr);
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
			RibbonWidget::InformativeToolButton* loadAndSaveAll = nullptr;
			RibbonWidget::InformativeToolButton* buildAll = nullptr;
		};

		struct GitButtons
		{
			RibbonWidget::InformativeToolButton* pullProjects = nullptr;
			RibbonWidget::InformativeToolButton* pushProjects = nullptr;
		};


		static TemplateManagementButtons& getTemplateManagementButtons();
		static ProjectButtons& getProjectButtons();
		static GitButtons& getGitButtons();
	private:
		static RibbonImpl* m_instance;

		TemplateManagementButtons m_templateButtons;
		ProjectButtons m_projectButtons;
		GitButtons m_gitButtons;

		RibbonWidget::RibbonTab* m_mainTab;

		RibbonWidget::RibbonButtonGroup* m_templateGroup;
		RibbonWidget::RibbonButtonGroup* m_projectGroup;
		RibbonWidget::RibbonButtonGroup* m_gitGroup;

	};
}
