/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICESESSION_TEST_H
#define LANGUAGESERVICESESSION_TEST_H

#include <QObject>

class LanguageServiceSessionTest : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void digestifLikeInitialize();
	void synchronizationMapping_data();
	void synchronizationMapping();
	void openCloseMapping_data();
	void openCloseMapping();
	void featureMapping();
	void completionRequestMapping();
	void completionResults_data();
	void completionResults();
	void definitionRequestMapping();
	void definitionResults_data();
	void definitionResults();
	void unknownCapabilitiesIgnored();
	void positionEncoding_data();
	void positionEncoding();
	void initializeFailure_data();
	void initializeFailure();
	void processExitWhileInitializing();
	void unexpectedExitAfterReady();
	void unsupportedServerRequestIsRejected();
	void gracefulShutdown();
	void shutdownTimeoutIsBounded();
	void stopDuringInitialization();
	void sessionGenerationIsDistinct();
	void managerOwnership();
	void managerPendingReplacementOwnership();
	void documentLifecycleMapping();
	void optionalConfiguredServer();
};

#endif // LANGUAGESERVICESESSION_TEST_H
