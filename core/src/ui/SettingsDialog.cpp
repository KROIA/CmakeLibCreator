#include "ui/SettingsDialog.h"
#include "Resources.h"

namespace CLC
{
	SettingsDialog::SettingsDialog(QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
		setWindowTitle("Settings");
		hide();
	}

	SettingsDialog::~SettingsDialog()
	{
	}

	void SettingsDialog::on_apply_pushButton_clicked()
	{
		saveSettings();
		hide();
	}
	void SettingsDialog::on_cancel_pushButton_clicked()
	{
		hide();
	}
	void SettingsDialog::loadSettings()
	{
		const Resources::GitResources& gitResources = Resources::getTemplateGitRepo();

		ui.templateRepo_lineEdit->setText(gitResources.repo);
		ui.templateBranch_lineEdit->setText(gitResources.templateBranch);
		ui.dependenciesBranch_lineEdit->setText(gitResources.dependenciesBranch);
		ui.qtModulesBranch_lineEdit->setText(gitResources.qtModulesBranch);
	}
	void SettingsDialog::saveSettings()
	{
		Resources::GitResources gitResources;
		gitResources.repo = ui.templateRepo_lineEdit->text();
		gitResources.templateBranch = ui.templateBranch_lineEdit->text();
		gitResources.dependenciesBranch = ui.dependenciesBranch_lineEdit->text();
		gitResources.qtModulesBranch = ui.qtModulesBranch_lineEdit->text();
		Resources::setTemplateGitRepo(gitResources);
		Resources::saveSettings();
	}
	void SettingsDialog::showEvent(QShowEvent* event)
	{
		loadSettings();
		QWidget::showEvent(event);
	}
}
