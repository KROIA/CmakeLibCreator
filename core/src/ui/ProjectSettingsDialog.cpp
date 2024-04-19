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

		m_qtMajorVersionText[5] = "Qt5";
		m_qtMajorVersionText[6] = "Qt6";

		m_qtModulesDialog = new CheckBoxSelectionDialog("QT Modules");
		m_dependenciesDialog = new CheckBoxSelectionDialog("Dependencies");

		connect(m_qtModulesDialog, &CheckBoxSelectionDialog::okButtonClicked, this, &ProjectSettingsDialog::onQtModulesSelected);
		connect(m_dependenciesDialog, &CheckBoxSelectionDialog::okButtonClicked, this, &ProjectSettingsDialog::onDependenciesSelected);

		ui.qt_majorVersion_comboBox->clear();
		for (const auto& v : m_qtMajorVersionText)
			ui.qt_majorVersion_comboBox->addItem(v.second);
	}

	ProjectSettingsDialog::~ProjectSettingsDialog()
	{
		delete m_qtModulesDialog;
		delete m_dependenciesDialog;
	}

	void ProjectSettingsDialog::setSettings(const ProjectSettings& settings)
	{
		m_ignoreNameChangeEvents = true;
		m_settings = settings;
		const ProjectSettings::LibrarySettings &libSettings = m_settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = m_settings.getCMAKE_settings();
		ui.libraryName_lineEdit->setText(cmakeSettings.libraryName);
		ui.namespaceName_lineEdit->setText(libSettings.namespaceName);
		QString exportSubstr = libSettings.exportName;
		int idx = exportSubstr.indexOf("_EXPORT");
		if (idx != -1)
		{
			exportSubstr = exportSubstr.mid(0, idx);
		}
		ui.exportName_lineEdit->setText(exportSubstr);
		ui.libraryNameShort_lineEdit->setText(cmakeSettings.lib_short_define);
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


		if(cmakeSettings.qt_installBase.isEmpty())
			ui.qt_installBasePath_lineEdit->setText("C:/Qt");
		else
			ui.qt_installBasePath_lineEdit->setText(cmakeSettings.qt_installBase);

		QStringList parts = cmakeSettings.qt_version.split('.');
		int major = cmakeSettings.qt_major_version;
		

		int minor = 0;
		int patch = 0;
		bool useNewestQtVersion = false;
		if (parts.size() == 3) {
			major = parts[0].toInt();
			minor = parts[1].toInt();
			patch = parts[2].toInt();
		}
		else {
			useNewestQtVersion = true;
		}
		if (major <= 0)
			major = 5;
		ui.qt_useNewestVersion_checkBox->setChecked(useNewestQtVersion);
		on_qt_useNewestVersion_checkBox_clicked(useNewestQtVersion);
		ui.qt_minorVersion_spinBox->setValue(minor);
		ui.qt_patchVersion_spinBox->setValue(patch);
		if (m_qtMajorVersionText.find(major) != m_qtMajorVersionText.end())
		{
			int index = 0;
			for (const auto& el : m_qtMajorVersionText)
			{
				if (el.first == major)
					break;
				++index;
			}
			if(index < ui.qt_majorVersion_comboBox->count())
				ui.qt_majorVersion_comboBox->setCurrentIndex(index);
		}
		if(cmakeSettings.qt_compiler.isEmpty())
			ui.qt_compilerName_lineEdit->setText("autoFind");
		else
			ui.qt_compilerName_lineEdit->setText(cmakeSettings.qt_compiler);

		m_ignoreNameChangeEvents = false;
	}
	const ProjectSettings& ProjectSettingsDialog::getSettings()
	{
		ProjectSettings::LibrarySettings libSettings = m_settings.getLibrarySettings();
		libSettings.version.major = ui.major_spinBox->value();
		libSettings.version.minor = ui.minor_spinBox->value();
		libSettings.version.patch = ui.patch_spinBox->value();

		libSettings.author = ui.author_lineEdit->text();
		libSettings.email = ui.email_lineEdit->text();
		libSettings.website = ui.website_lineEdit->text();
		libSettings.license = ui.license_lineEdit->text();

		libSettings.namespaceName = ui.namespaceName_lineEdit->text();
		libSettings.exportName = ui.exportName_lineEdit->text() + "_EXPORT";

		ProjectSettings::CMAKE_settings cmakeSettings = m_settings.getCMAKE_settings();
		cmakeSettings.libraryName = ui.libraryName_lineEdit->text();
		cmakeSettings.lib_define = ui.libDefine_lineEdit->text();
		cmakeSettings.lib_profile_define = ui.libProfileDefine_lineEdit->text();
		cmakeSettings.lib_short_define = ui.libraryNameShort_lineEdit->text();
		cmakeSettings.qt_enable = ui.qtEnable_checkBox->isChecked();
		cmakeSettings.qt_deploy = ui.qtDeploy_checkBox->isChecked();
		cmakeSettings.debugPostFix = ui.debugPostFix_lineEdit->text();
		cmakeSettings.staticPostFix = ui.staticPostFix_lineEdit->text();
		cmakeSettings.profilingPostFix = ui.profilingPostFix_lineEdit->text();
		cmakeSettings.cxxStandard = ui.cxxStandard_spinBox->value();
		cmakeSettings.compile_examples = ui.compileExamples_checkBox->isChecked();
		cmakeSettings.compile_unittests = ui.compileUnitTests_checkBox->isChecked();


		cmakeSettings.qt_installBase = ui.qt_installBasePath_lineEdit->text();
		QString majorStr = ui.qt_majorVersion_comboBox->currentText().toLower();
		int major = majorStr.mid(majorStr.indexOf("qt") + 2).toInt();
		cmakeSettings.qt_major_version = major;
		if (ui.qt_useNewestVersion_checkBox->isChecked())
			cmakeSettings.qt_version = "autoFind";
		else
		{
			int minor = ui.qt_minorVersion_spinBox->value();
			int patch = ui.qt_patchVersion_spinBox->value();
			cmakeSettings.qt_version = QString::number(major) + "." + QString::number(minor) + "." + QString::number(patch);
		}
		QString qtCompilerName = ui.qt_compilerName_lineEdit->text();
		if (qtCompilerName.isEmpty())
			qtCompilerName = "autoFind";
		cmakeSettings.qt_compiler = qtCompilerName;

		
		m_settings.setLibrarySettings(libSettings);
		m_settings.setCMAKE_settings(cmakeSettings);

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
		const QVector<Dependency>& dependencies = Resources::getDependencies();
		const QVector<Dependency>& currentlyUsedDependencies = m_settings.getCMAKE_settings().dependencies;
		QVector<CheckBoxSelectionDialog::Element> elements;
		for (const Dependency& dep : dependencies)
		{
			CheckBoxSelectionDialog::Element e;
			e.name = dep.getName();
			e.tooltip = dep.getDescription();
			e.selected = false;
			e.color = QColor(255, 255, 255);
			elements.push_back(e);
		}
		QVector<bool> selected(currentlyUsedDependencies.size(), false);
		for (int i=0; i<currentlyUsedDependencies.size(); ++i)
		{
			for (CheckBoxSelectionDialog::Element& e : elements)
			{
				if (e.name == currentlyUsedDependencies[i].getName())
				{
					e.selected = true;
					selected[i] = true;
					break;
				}
			}
		}
		// Add dependencies that are not in the official list but are still used
		for (int i = 0; i < selected.size(); ++i)
		{
			if (!selected[i])
			{
				CheckBoxSelectionDialog::Element e;
				e.name = currentlyUsedDependencies[i].getName();
				QString desc = currentlyUsedDependencies[i].getDescription();
				if (desc.size() == 0)
				{
					desc = "No description available";
				}
				e.tooltip = desc;
				e.selected = true;
				e.color = QColor(255, 255, 0);
				elements.push_back(e);
			}
		}
		m_dependenciesDialog->setItems(elements);
	}
	void ProjectSettingsDialog::on_libraryName_lineEdit_textChanged(const QString& text)
	{
		if (m_ignoreNameChangeEvents)
			return;
		ProjectSettings::CMAKE_settings cmakeSettings = m_settings.getCMAKE_settings();
		cmakeSettings.libraryName = text;
		m_settings.setCMAKE_settings(cmakeSettings);
		m_settings.autosetLibDefine();
		m_settings.autosetLibProfileDefine();
		m_settings.autosetLibShortDefine();
		m_settings.autoSetNamespaceName();
		m_settings.autoSetExportName();
		const ProjectSettings::LibrarySettings& libSettings = m_settings.getLibrarySettings();
		cmakeSettings = m_settings.getCMAKE_settings();
		ui.libDefine_lineEdit->setText(cmakeSettings.lib_define);
		ui.libProfileDefine_lineEdit->setText(cmakeSettings.lib_profile_define);
		ui.namespaceName_lineEdit->setText(libSettings.namespaceName);
		QString exportSubstr = libSettings.exportName;
		int idx = exportSubstr.indexOf("_EXPORT");
		if (idx != -1)
		{
			exportSubstr = exportSubstr.mid(0, idx);
		}
		ui.exportName_lineEdit->setText(exportSubstr);
		ui.libraryNameShort_lineEdit->setText(cmakeSettings.lib_short_define);
	}

	void ProjectSettingsDialog::on_qt_useNewestVersion_checkBox_clicked(bool checked)
	{
		ui.qt_minorVersion_spinBox->setEnabled(!checked);
		ui.qt_patchVersion_spinBox->setEnabled(!checked);
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
