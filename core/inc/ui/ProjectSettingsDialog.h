#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>
#include "ui_ProjectSettingsDialog.h"
#include "ProjectSettings.h"

namespace CLC
{

	class ProjectSettingsDialog : public QWidget
	{
		Q_OBJECT

	public:
		ProjectSettingsDialog(QWidget* parent = nullptr);
		~ProjectSettingsDialog();

		void setSettings(const ProjectSettings& settings);
		const ProjectSettings &getSettings() const;

	private:
		Ui::ProjectSettingsDialog ui;

		ProjectSettings m_settings;
	};
}
