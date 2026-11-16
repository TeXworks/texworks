/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICENAVIGATIONWINDOW_TEST_H
#define LANGUAGESERVICENAVIGATIONWINDOW_TEST_H

#include <QObject>

class LanguageServiceNavigationWindowTest : public QObject
{
	Q_OBJECT

public slots:
	void acceptLanguageServicesPreferences();

private slots:
	void engineMetadataPersistence();
	void staticCompletionRegression();
	void completionCandidateList();
	void completionCandidateListDeactivation();
	void completionCandidateListProviderIntegration();
	void completionPreviewUndoRedoSynchronization();
	void productionConfigurationActivation();
	void productionConfigurationFailures();
	void pathCommandActivation();
	void preferencesLanguageServicesUi();
	void optionalConfiguredProductionServer();
	void actionAndNavigation();
};

#endif // LANGUAGESERVICENAVIGATIONWINDOW_TEST_H
