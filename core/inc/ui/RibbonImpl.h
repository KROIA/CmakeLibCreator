#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RibbonWidget.h"

namespace CLC {
	class RibbonImpl : public RibbonWidget::Ribbon
	{
		Q_OBJECT
	public:
		RibbonImpl(QWidget* parent = nullptr);



		struct SettingsButtons
		{
			RibbonWidget::RibbonButton* settings1;
			RibbonWidget::RibbonButton* settingsOK;
			RibbonWidget::RibbonButton* settingsCancel;
			RibbonWidget::RibbonButton* settingsSave;
		};

		struct WorkButtons
		{
			RibbonWidget::InformativeToolButton* open;
			RibbonWidget::InformativeToolButton* save;
		};

		struct EditButtons
		{
			RibbonWidget::RibbonButton* undo;
			RibbonWidget::RibbonButton* redo;
		};


		SettingsButtons& settingsButtons();
		WorkButtons& workButtons();
		EditButtons& editButtons();
	private:
		SettingsButtons m_settingsButtons;
		WorkButtons m_workButtons;
		EditButtons m_editButtons;
	};
}