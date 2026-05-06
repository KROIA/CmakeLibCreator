#include "ui/SettingsDialog.h"
#include "Resources.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace CLC
{
	namespace
	{
		const char* kRowLineEditProperty = "lineEditPtr";
	}

	SettingsDialog::SettingsDialog(QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
		setWindowTitle("Settings");

		// The rows layout was created inside the scroll-area's contents widget.
		// We keep that layout and insert dynamic rows above its trailing spacer.
		m_projectPathsLayout = ui.projectPaths_rowsLayout;

		ui.defaultLibraryPathBrowse_toolButton->setIcon(QIcon(":/icons/folder-open.png"));

		// Note: the two `on_<widget>_clicked` slots below are wired automatically by
		// Qt's connectSlotsByName() inside ui.setupUi(this). Adding manual connect()
		// calls here would cause each click to fire the slot twice.

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
	void SettingsDialog::on_defaultLibraryPathBrowse_toolButton_clicked()
	{
		const QString current = ui.defaultLibraryPath_lineEdit->text();
		const QString start = current.isEmpty() ? Resources::getDefaultLibraryPath() : current;
		const QString picked = QFileDialog::getExistingDirectory(this, tr("Default library path"), start,
			QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (!picked.isEmpty())
			ui.defaultLibraryPath_lineEdit->setText(picked);
	}
	void SettingsDialog::on_addProjectPath_pushButton_clicked()
	{
		addProjectPathRow(QString());
	}
	void SettingsDialog::loadSettings()
	{
		const Resources::GitResources& gitResources = Resources::getTemplateGitRepo();

		ui.templateRepo_lineEdit->setText(gitResources.repo);
		ui.templateBranch_lineEdit->setText(gitResources.templateBranch);
		ui.dependenciesBranch_lineEdit->setText(gitResources.dependenciesBranch);
		ui.qtModulesBranch_lineEdit->setText(gitResources.qtModulesBranch);

		ui.defaultLibraryPath_lineEdit->setText(Resources::getDefaultLibraryPath());

		clearProjectPathRows();
		const Resources::LoadSaveProjects& projects = Resources::getLoadSaveProjects();
		for (const QString& p : projects.projectPaths)
			addProjectPathRow(p);
	}
	void SettingsDialog::saveSettings()
	{
		Resources::GitResources gitResources;
		gitResources.repo = ui.templateRepo_lineEdit->text();
		gitResources.templateBranch = ui.templateBranch_lineEdit->text();
		gitResources.dependenciesBranch = ui.dependenciesBranch_lineEdit->text();
		gitResources.qtModulesBranch = ui.qtModulesBranch_lineEdit->text();
		Resources::setTemplateGitRepo(gitResources);

		Resources::setDefaultLibraryPath(ui.defaultLibraryPath_lineEdit->text());

		Resources::LoadSaveProjects projects;
		if (m_projectPathsLayout)
		{
			for (int i = 0; i < m_projectPathsLayout->count(); ++i)
			{
				QLayoutItem* item = m_projectPathsLayout->itemAt(i);
				if (!item)
					continue;
				QWidget* row = item->widget();
				if (!row)
					continue;
				QLineEdit* edit = row->property(kRowLineEditProperty).value<QLineEdit*>();
				if (!edit)
					continue;
				const QString text = edit->text().trimmed();
				if (!text.isEmpty())
					projects.projectPaths.push_back(text);
			}
		}
		Resources::setLoadSaveProjects(projects);
		Resources::saveSettings();
	}
	void SettingsDialog::addProjectPathRow(const QString& initial)
	{
		if (!m_projectPathsLayout)
			return;

		QWidget* row = new QWidget(ui.projectPaths_scrollContents);
		QHBoxLayout* h = new QHBoxLayout(row);
		h->setContentsMargins(0, 0, 0, 0);

		QLineEdit* edit = new QLineEdit(row);
		edit->setText(initial);

		QToolButton* browse = new QToolButton(row);
		browse->setIcon(QIcon(":/icons/folder-open.png"));

		QToolButton* del = new QToolButton(row);
		del->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

		h->addWidget(edit);
		h->addWidget(browse);
		h->addWidget(del);

		row->setProperty(kRowLineEditProperty, QVariant::fromValue<QLineEdit*>(edit));

		// Insert above the trailing spacer (always last item).
		const int insertIndex = m_projectPathsLayout->count() > 0 ? m_projectPathsLayout->count() - 1 : 0;
		m_projectPathsLayout->insertWidget(insertIndex, row);

		connect(browse, &QToolButton::clicked, this, [this, edit]()
		{
			const QString current = edit->text();
			const QString start = current.isEmpty() ? Resources::getDefaultLibraryPath() : current;
			const QString picked = QFileDialog::getExistingDirectory(this, tr("Project path"), start,
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
			if (!picked.isEmpty())
				edit->setText(picked);
		});
		connect(del, &QToolButton::clicked, this, [this, row]()
		{
			if (!m_projectPathsLayout)
				return;
			m_projectPathsLayout->removeWidget(row);
			row->deleteLater();
		});
	}
	void SettingsDialog::clearProjectPathRows()
	{
		if (!m_projectPathsLayout)
			return;
		// Walk in reverse and delete widgets; keep the trailing spacer (item without widget).
		for (int i = m_projectPathsLayout->count() - 1; i >= 0; --i)
		{
			QLayoutItem* item = m_projectPathsLayout->itemAt(i);
			if (!item)
				continue;
			QWidget* w = item->widget();
			if (!w)
				continue;
			m_projectPathsLayout->removeWidget(w);
			w->deleteLater();
		}
	}
	void SettingsDialog::showEvent(QShowEvent* event)
	{
		loadSettings();
		QWidget::showEvent(event);
	}
}
