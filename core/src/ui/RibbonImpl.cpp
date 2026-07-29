#include "ui/RibbonImpl.h"

namespace CLC {
	RibbonImpl* RibbonImpl::m_instance = nullptr;
	RibbonImpl::RibbonImpl(QToolBar* parent)
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


		// Create buttons
		m_templateButtons.openTemplatePath = new RibbonWidget::RibbonButton("Open template path", "Open an existing folder which holds the template for libraries.", ":/icons/folder-open.png", true, m_templateGroup);
		m_templateButtons.downloadTemplate = new RibbonWidget::InformativeToolButton("Download template", "Download the newest version of the template.", ":/icons/download.png", true, m_templateGroup);

		m_projectButtons.openExistingProject = new RibbonWidget::InformativeToolButton("Open", "Open an existing library", ":/icons/folder-open.png", true, m_projectGroup);
		m_projectButtons.saveExistingProject = new RibbonWidget::InformativeToolButton("Save", "Save to existing library", ":/icons/save.png", true, m_projectGroup);
		m_projectButtons.saveAsNewProject    = new RibbonWidget::InformativeToolButton("Save as new", "Save as new library project", ":/icons/save.png", true, m_projectGroup);

		// Repositories tab
		m_repoTab = new RibbonWidget::RibbonTab("Repositories", "", this);
		m_repoGeneralGroup = new RibbonWidget::RibbonButtonGroup("General", m_repoTab);
		m_repoGitGroup     = new RibbonWidget::RibbonButtonGroup("Git", m_repoTab);
		m_repoBuildGroup   = new RibbonWidget::RibbonButtonGroup("Build", m_repoTab);

		m_repositoryButtons.openFolders     = new RibbonWidget::InformativeToolButton("Open folders", "Opens the project folder of each selected repository in a new Explorer window", ":/icons/folder-open.png", true, m_repoGeneralGroup);
		m_repositoryButtons.updateTemplates = new RibbonWidget::InformativeToolButton("Update templates", "Updates the library template of all selected repositories", ":/icons/save.png", true, m_repoGeneralGroup);
		m_repositoryButtons.refreshStatus   = new RibbonWidget::InformativeToolButton("Refresh status", "Re-reads git status, commit message, library and template version of all repositories", ":/icons/search.png", true, m_repoGeneralGroup);

		m_repositoryButtons.pull    = new RibbonWidget::InformativeToolButton("Pull", "git pull on all selected repositories", ":/icons/download.png", true, m_repoGitGroup);
		m_repositoryButtons.push    = new RibbonWidget::InformativeToolButton("Push", "git push on all selected repositories", ":/icons/upload.png", true, m_repoGitGroup);
		m_repositoryButtons.commit  = new RibbonWidget::InformativeToolButton("Commit", "Commit all selected repositories with the same message", ":/icons/accept.png", true, m_repoGitGroup);
		m_repositoryButtons.discard = new RibbonWidget::InformativeToolButton("Discard", "Discards uncommitted changes of all selected repositories", "", true, m_repoGitGroup);

		m_repositoryButtons.build    = new RibbonWidget::InformativeToolButton("Build", "Builds all selected repositories sequentially", ":/icons/hammer.png", true, m_repoBuildGroup);
		m_repositoryButtons.clean    = new RibbonWidget::InformativeToolButton("Clean", "Deletes the build folder of all selected repositories", "", true, m_repoBuildGroup);
		m_repositoryButtons.unitTest = new RibbonWidget::InformativeToolButton("Unittest", "Runs the unittests of all selected repositories", ":/icons/accept.png", true, m_repoBuildGroup);

		// Add tabs
		addTab(m_mainTab);
		addTab(m_repoTab);
	}
	RibbonImpl::~RibbonImpl()
	{
		m_instance = nullptr;
		delete m_templateButtons.openTemplatePath;
		delete m_templateButtons.downloadTemplate;
		delete m_projectButtons.openExistingProject;
		delete m_projectButtons.saveExistingProject;
		delete m_projectButtons.saveAsNewProject;
		delete m_repositoryButtons.openFolders;
		delete m_repositoryButtons.updateTemplates;
		delete m_repositoryButtons.refreshStatus;
		delete m_repositoryButtons.pull;
		delete m_repositoryButtons.push;
		delete m_repositoryButtons.commit;
		delete m_repositoryButtons.discard;
		delete m_repositoryButtons.build;
		delete m_repositoryButtons.clean;
		delete m_repositoryButtons.unitTest;
		delete m_templateGroup;
		delete m_projectGroup;
		delete m_repoGeneralGroup;
		delete m_repoGitGroup;
		delete m_repoBuildGroup;
		delete m_mainTab;
		delete m_repoTab;

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
	RibbonImpl::RepositoryButtons& RibbonImpl::getRepositoryButtons()
	{
		if (m_instance)
			return m_instance->m_repositoryButtons;
		static RepositoryButtons empty;
		return empty;
	}

}
