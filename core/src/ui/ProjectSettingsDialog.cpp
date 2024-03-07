#include "ui/ProjectSettingsDialog.h"

namespace CLC
{
	ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
	}

	ProjectSettingsDialog::~ProjectSettingsDialog()
	{
	}

	void ProjectSettingsDialog::setSettings(const ProjectSettings& settings)
	{
		m_settings = settings;
	}
	const ProjectSettings& ProjectSettingsDialog::getSettings() const
	{
		return m_settings;
	}
}