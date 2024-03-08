#include "ui/ProjectSettingsDialog.h"
#include "QTModule.h"
#include "Dependency.h"
#include "Resources.h"

namespace CLC
{
	ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);

		m_qtModulesDialog = new CheckBoxSelectionDialog("QT Modules");
		m_dependenciesDialog = new CheckBoxSelectionDialog("Dependencies");
	}

	ProjectSettingsDialog::~ProjectSettingsDialog()
	{
		delete m_qtModulesDialog;
		delete m_dependenciesDialog;
	}

	void ProjectSettingsDialog::setSettings(const ProjectSettings& settings)
	{
		m_settings = settings;
		const ProjectSettings::LibrarySettings &libSettings = m_settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = m_settings.getCMAKE_settings();
		ui.libraryName_lineEdit->setText(cmakeSettings.libraryName);
		ui.major_spinBox->setValue(libSettings.version.major);
		ui.minor_spinBox->setValue(libSettings.version.minor);
		ui.patch_spinBox->setValue(libSettings.version.patch);
		ui.author_lineEdit->setText(libSettings.author);
		ui.email_lineEdit->setText(libSettings.email);
		ui.website_lineEdit->setText(libSettings.website);
		ui.license_lineEdit->setText(libSettings.license);

		ui.libDefine_lineEdit->setText(cmakeSettings.lib_define);
		ui.libProfileDefine_lineEdit->setText(cmakeSettings.lib_profile_define);
		ui.qtEnable_checkBox->setChecked(cmakeSettings.qt_enable);
		ui.qtDeploy_checkBox->setChecked(cmakeSettings.qt_deploy);

		ui.debugPostFix_lineEdit->setText(cmakeSettings.debugPostFix);
		ui.staticPostFix_lineEdit->setText(cmakeSettings.staticPostFix);
		ui.profilingPostFix_lineEdit->setText(cmakeSettings.profilingPostFix);

		ui.cxxStandard_spinBox->setValue(cmakeSettings.cxxStandard);
		ui.compileExamples_checkBox->setChecked(cmakeSettings.compile_examples);
		ui.compileUnitTests_checkBox->setChecked(cmakeSettings.compile_unittests);

	}
	const ProjectSettings& ProjectSettingsDialog::getSettings() const
	{
		return m_settings;
	}

	void ProjectSettingsDialog::on_qtModules_pushButton_clicked()
	{
		QVector<QTModule> qtModules = Resources::getQTModules();
		const QVector<QTModule> &currentlyUsedModules = m_settings.getCMAKE_settings().qModules;
		QVector<CheckBoxSelectionDialog::Element> elements;
		for (const QTModule& module : qtModules)
		{
			CheckBoxSelectionDialog::Element e;
			e.name = module.getName();
			e.tooltip = module.getDescription();
			e.selected = false;
			elements.push_back(e);
		}
		for(const QTModule& module : currentlyUsedModules)
		{
			for (CheckBoxSelectionDialog::Element& e : elements)
			{
				if (e.name == module.getName())
				{
					e.selected = true;
					break;
				}
			}
		}
		m_qtModulesDialog->setItems(elements);
	}
	void ProjectSettingsDialog::on_dependencies_pushButton_clicked()
	{
		QVector<Dependency> dependencies = Resources::getDependencies();
		const QVector<Dependency>& currentlyUsedDependencies = m_settings.getCMAKE_settings().dependencies;
		QVector<CheckBoxSelectionDialog::Element> elements;
		for (const Dependency& dep : dependencies)
		{
			CheckBoxSelectionDialog::Element e;
			e.name = dep.getName();
			e.tooltip = dep.getDescription();
			e.selected = false;
			elements.push_back(e);
		}
		for (const Dependency& dep : currentlyUsedDependencies)
		{
			for (CheckBoxSelectionDialog::Element& e : elements)
			{
				if (e.name == dep.getName())
				{
					e.selected = true;
					break;
				}
			}
		}
		m_dependenciesDialog->setItems(elements);
	}

	void ProjectSettingsDialog::onQtModulesSelected(const QVector<CheckBoxSelectionDialog::Element>& selectedItems)
	{
		ProjectSettings::CMAKE_settings cmakeSettings = m_settings.getCMAKE_settings();
		cmakeSettings.qModules.clear();
		for (const CheckBoxSelectionDialog::Element& e : selectedItems)
		{
			if (e.selected)
			{
				cmakeSettings.qModules.push_back(QTModule(e.name, ""));
			}
		}
		m_settings.setCMAKE_settings(cmakeSettings);
	}
	void ProjectSettingsDialog::onDependenciesSelected(const QVector<CheckBoxSelectionDialog::Element>& selectedItems)
	{
		ProjectSettings::CMAKE_settings cmakeSettings = m_settings.getCMAKE_settings();
		cmakeSettings.dependencies.clear();
		for (const CheckBoxSelectionDialog::Element& e : selectedItems)
		{
			if (e.selected)
			{
				cmakeSettings.dependencies.push_back(Dependency(e.name, ""));
			}
		}
		m_settings.setCMAKE_settings(cmakeSettings);
	}
}