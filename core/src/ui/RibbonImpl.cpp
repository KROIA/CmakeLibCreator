#include "ui/RibbonImpl.h"

namespace CLC {
	RibbonImpl::RibbonImpl(QWidget* parent)
		: RibbonWidget::Ribbon(parent)
	{
		// Create tabs
		RibbonWidget::RibbonTab* tabSettings = new RibbonWidget::RibbonTab("Settings", "developerTool", this);
		RibbonWidget::RibbonTab* tabEdit = new RibbonWidget::RibbonTab("Editing", "create_new_2", this);

		// Create groups
		RibbonWidget::RibbonButtonGroup* groupSettings = new RibbonWidget::RibbonButtonGroup("group\nSettings", tabSettings);
		RibbonWidget::RibbonButtonGroup* groupWork = new RibbonWidget::RibbonButtonGroup("groupWork", tabEdit);
		RibbonWidget::RibbonButtonGroup* groupEdit = new RibbonWidget::RibbonButtonGroup("groupEdit", tabEdit);

		// Create buttons
		m_settingsButtons.settings1 = new RibbonWidget::RibbonButton("Settings 1", "Settings 1", "engineering_1", true, groupSettings);
		m_settingsButtons.settingsOK = new RibbonWidget::RibbonButton("Settings OK", "Settings OK", "accept", true, groupSettings);
		m_settingsButtons.settingsCancel = new RibbonWidget::RibbonButton("Cancel", "Cancel", "close", true, groupSettings);
		m_settingsButtons.settingsSave = new RibbonWidget::RibbonButton("Settings Save", "Settings Save", "floppy_disk", true, groupSettings);

		m_workButtons.open = new RibbonWidget::InformativeToolButton("Open", "Open", "document", true, groupWork);
		m_workButtons.save = new RibbonWidget::InformativeToolButton("Save", "Save", "floppy_disk", true, groupWork);

		m_editButtons.undo = new RibbonWidget::RibbonButton("Undo", "Undo", "arrowCClockwise", true, groupEdit);
		m_editButtons.redo = new RibbonWidget::RibbonButton("Redo", "Redo", "arrowClockwise", true, groupEdit);

		// Add tabs
		addTab(tabSettings);
		addTab(tabEdit);
	}

	RibbonImpl::SettingsButtons& RibbonImpl::settingsButtons()
	{
		return m_settingsButtons;
	}
	RibbonImpl::WorkButtons& RibbonImpl::workButtons()
	{
		return m_workButtons;
	}
	RibbonImpl::EditButtons& RibbonImpl::editButtons()
	{
		return m_editButtons;
	}
}