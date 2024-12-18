#include "ui/RibbonImpl.h"

namespace CLC {
	RibbonImpl* RibbonImpl::m_instance = nullptr;
	RibbonImpl::RibbonImpl(QWidget* parent)
		: RibbonWidget::Ribbon(parent)
	{
		m_instance = this;
		Q_INIT_RESOURCE(AppIcons);
		// Create tabs
		m_mainTab = new RibbonWidget::RibbonTab("Project", "", this);
		//RibbonWidget::RibbonTab* tabEdit = new RibbonWidget::RibbonTab("Editing", "create_new_2", this);

		// Create groups
		m_templateGroup = new RibbonWidget::RibbonButtonGroup("Template management", m_mainTab);
		m_projectGroup = new RibbonWidget::RibbonButtonGroup("Library management", m_mainTab);
		m_gitGroup = new RibbonWidget::RibbonButtonGroup("Git", m_mainTab);


		// Create buttons
		m_templateButtons.openTemplatePath = new RibbonWidget::RibbonButton("Open template path", "Open an existing folder which holds the template for libraries.", ":/icons/folder-open.png", true, m_templateGroup);
		m_templateButtons.downloadTemplate = new RibbonWidget::InformativeToolButton("Download template", "Download the newest version of the template.", ":/icons/download.png", true, m_templateGroup);

		m_projectButtons.openExistingProject = new RibbonWidget::InformativeToolButton("Open", "Open an existing library", ":/icons/folder-open.png", true, m_projectGroup);
		m_projectButtons.saveExistingProject = new RibbonWidget::InformativeToolButton("Save", "Save to existing library", ":/icons/save.png", true, m_projectGroup);
		m_projectButtons.saveAsNewProject    = new RibbonWidget::InformativeToolButton("Save as new", "Save as new library project", ":/icons/save.png", true, m_projectGroup);
		m_projectButtons.loadAndSaveAll		 = new RibbonWidget::InformativeToolButton("Load and save all", "Loads all projects and saves them back with the new template", ":/icons/save.png", true, m_projectGroup);
		m_projectButtons.buildAll			 = new RibbonWidget::InformativeToolButton("Build all", "Builds all projects", ":/icons/hammer.png", true, m_projectGroup);

		m_gitButtons.pullProjects = new RibbonWidget::InformativeToolButton("Pull", "Pull projects from the git repository", ":/icons/download.png", true, m_gitGroup);
		m_gitButtons.pushProjects = new RibbonWidget::InformativeToolButton("Push", "Push projects to the git repository", ":/icons/upload.png", true, m_gitGroup);

		// Add tabs
		addTab(m_mainTab);
	}
	RibbonImpl::~RibbonImpl()
	{
		m_instance = nullptr;
		delete m_templateButtons.openTemplatePath;
		delete m_templateButtons.downloadTemplate;
		delete m_projectButtons.openExistingProject;
		delete m_projectButtons.saveExistingProject;
		delete m_projectButtons.saveAsNewProject;
		delete m_projectButtons.loadAndSaveAll;
		delete m_projectButtons.buildAll;
		delete m_gitButtons.pullProjects;
		delete m_gitButtons.pushProjects;
		delete m_templateGroup;
		delete m_projectGroup;
		delete m_gitGroup;
		delete m_mainTab;
		
	}

	RibbonImpl::TemplateManagementButtons& RibbonImpl::getTemplateManagementButtons()
	{
		if(m_instance)
			return m_instance->m_templateButtons;
		static TemplateManagementButtons empty;
		return empty;
	}
	RibbonImpl::ProjectButtons& RibbonImpl::getProjectButtons()
	{
		if (m_instance)
			return m_instance->m_projectButtons;
		static ProjectButtons empty;
		return empty;
	}
	RibbonImpl::GitButtons& RibbonImpl::getGitButtons()
	{
		if (m_instance)
			return m_instance->m_gitButtons;
		static GitButtons empty;
		return empty;
	}

}
