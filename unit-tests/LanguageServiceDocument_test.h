/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICEDOCUMENT_TEST_H
#define LANGUAGESERVICEDOCUMENT_TEST_H

#include <QObject>

class LanguageServiceDocumentTest : public QObject
{
	Q_OBJECT

private slots:
	void eligibility();
	void texIntentChanges();
	void deferredRefreshIsolation();
	void deferredRefreshLifecycle();
	void unsavedThenStored();
	void openCloseCapability();
	void syncNone();
	void fullSynchronization();
	void incrementalChanges_data();
	void incrementalChanges();
	void undoRedo();
	void sameIdentityIsSilent();
	void identityAndEligibilityTransitions();
	void reloadSynchronizes();
	void serviceLifecycleAndGeneration();
	void replacementKeepsDocumentBinding();
	void closeLifecycle();
	void multipleDocumentsShareService();
	void completionCapabilityAndRequestContext();
	void completionFreshness();
	void definitionCapabilityAndRequestContext();
	void definitionFreshness();
	void definitionUnavailableDocuments();
	void definitionRangeConversion();
};

#endif // LANGUAGESERVICEDOCUMENT_TEST_H
