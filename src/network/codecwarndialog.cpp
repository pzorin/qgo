#include <QtGui>
#include "codecwarndialog.h"
#include "../defines.h"

CodecWarnDialog::CodecWarnDialog(const char * encoding)
{
	if(!preferences.warn_about_codecs)
	{
		deleteLater();
		done(0);
	}
	/* FIXME okayButton should be on lower right and window should be slightly
	 * larger, more warning like, maybe with padding around text, some kind of
	 * layout. */
	okayButton = new QPushButton(tr("Okay"));
	okayButton->setMaximumWidth(80);
	okayButton->setDefault(true);
	
	dontwarnCB = new QCheckBox(tr("Don't warn me again"));
	
	textLabel = new QLabel(tr("Can't find font codec \"%1\"\nUsing default").arg(encoding));
	
	connect(okayButton, SIGNAL(clicked()), this, SLOT(slot_okay()));
	
	QGridLayout * mainLayout = new QGridLayout;
	//mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(textLabel, 0, 0);
	mainLayout->addWidget(dontwarnCB, 1, 0);
	mainLayout->addWidget(okayButton, 2, 0);
	setLayout(mainLayout);
	
	setWindowTitle(tr("Missing Codec!"));
	show();
}

CodecWarnDialog::~CodecWarnDialog()
{
	if(dontwarnCB->isChecked())
		preferences.warn_about_codecs = 0;
	delete okayButton;
	delete dontwarnCB;
}

void CodecWarnDialog::slot_okay(void)
{
	/* Why does this need to be here as well as above? FIXME,
	 * why doesn't it suffice solely in deconstructor */
	if(dontwarnCB->isChecked())
		preferences.warn_about_codecs = 0;
	deleteLater();
	done(0);
}
