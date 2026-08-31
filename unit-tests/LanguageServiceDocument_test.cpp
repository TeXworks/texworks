/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "LanguageServiceDocument_test.h"

#include "document/TeXDocument.h"
#include "languageservices/LanguageServiceDocumentBinding.h"
#include "languageservices/LanguageServiceManager.h"
#include "languageservices/LanguageServiceNavigation.h"

#include <QDir>
#include <QFile>
#include <QPointer>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QtTest>

using namespace Tw::LanguageServices;

namespace {

struct Operation {
	enum Kind { Open, Change, Close, Completion, Definition } kind;
	QUrl url;
	QString languageId;
	quint64 version{0};
	QString text;
	bool hasRange{false};
	LanguageRange range;
	LanguageDefinitionRequest definitionRequest;
	LanguageCompletionRequest completionRequest;
};

class RecordingService : public LanguageService
{
public:
	explicit RecordingService(const LanguageServiceCapabilities & capabilities)
		: m_configuredCapabilities(capabilities) { }

	bool start() override
	{
		if (!beginStart())
			return false;
		setState(Initializing);
		becomeReady(m_configuredCapabilities);
		return true;
	}
	void stop() override
	{
		if (state() == Stopped)
			return;
		setState(Stopping);
		if (!holdStop)
			becomeStopped();
	}
	void finishStop() { becomeStopped(); }
	bool openDocument(const LanguageDocumentOpen & document) override
	{
		Operation operation;
		operation.kind = Operation::Open;
		operation.url = document.url;
		operation.languageId = document.languageId;
		operation.version = document.version;
		operation.text = document.text;
		operations.append(operation);
		return true;
	}
	bool changeDocument(const QUrl & url, quint64 version, const LanguageDocumentChange & change) override
	{
		Operation operation;
		operation.kind = Operation::Change;
		operation.url = url;
		operation.version = version;
		operation.text = change.text;
		operation.hasRange = change.hasRange;
		operation.range = change.range;
		operations.append(operation);
		return true;
	}
	bool closeDocument(const QUrl & url) override
	{
		Operation operation;
		operation.kind = Operation::Close;
		operation.url = url;
		operations.append(operation);
		return true;
	}
	bool requestCompletion(const LanguageCompletionRequest & request) override
	{
		Operation operation;
		operation.kind = Operation::Completion;
		operation.completionRequest = request;
		operations.append(operation);
		return acceptCompletionRequests;
	}
	bool requestDefinition(const LanguageDefinitionRequest & request) override
	{
		Operation operation;
		operation.kind = Operation::Definition;
		operation.definitionRequest = request;
		operations.append(operation);
		return acceptDefinitionRequests;
	}
	void finishDefinition(quint64 token, const QList<LanguageLocation> & locations)
	{
		emit definitionFinished(token, locations);
	}
	void finishCompletion(quint64 token, const QList<CompletionItem> & items)
	{
		emit completionFinished(token, items);
	}
	void failCompletionRequest(quint64 token)
	{
		emit completionFailed(token, QStringLiteral("scripted completion failure"));
	}
	void fail() { becomeFailed(QStringLiteral("scripted failure")); }

	QList<Operation> operations;
	bool acceptDefinitionRequests{true};
	bool acceptCompletionRequests{true};
	bool holdStop{false};

private:
	LanguageServiceCapabilities m_configuredCapabilities;
};

QString createFile(QTemporaryDir & directory, const QString & name)
{
	const QString path = QDir(directory.path()).filePath(name);
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly))
		return {};
	file.close();
	return path;
}

void store(Tw::Document::TeXDocument & document, const QString & path)
{
	document.documentLayout();
	document.setFileInfo(QFileInfo(path));
	document.setStoredInFilesystem(true);
}

LanguageServiceCapabilities capabilities(TextSyncKind sync = TextSyncKind::Incremental, bool openClose = true,
	                                     bool definition = false, bool completion = false)
{
	LanguageServiceCapabilities result;
	result.textSync = sync;
	result.openClose = openClose;
	result.definition = definition;
	result.completion = completion;
	return result;
}

RecordingService * configure(LanguageServiceManager & manager, const LanguageServiceCapabilities & caps)
{
	RecordingService * service = new RecordingService(caps);
	const bool configured = manager.setService(service, QStringList{QStringLiteral("context")});
	const bool started = configured && manager.start();
	Q_ASSERT(configured);
	Q_ASSERT(started);
	return service;
}

void comparePosition(const LanguagePosition & actual, int line, int character)
{
	QCOMPARE(actual.line, line);
	QCOMPARE(actual.character, character);
}

LanguagePosition languagePosition(int line, int character)
{
	LanguagePosition position;
	position.line = line;
	position.character = character;
	return position;
}

} // namespace

void LanguageServiceDocumentTest::eligibility()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	Tw::Document::TeXDocument document;
	QVERIFY(!LanguageServiceManager::isSourceEligible(&document));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document), QString());

	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	QVERIFY(LanguageServiceManager::isSourceEligible(&document));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document), QStringLiteral("context"));
	store(document, createFile(directory, QStringLiteral("a.mkiv")));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document), QStringLiteral("context"));
	store(document, createFile(directory, QStringLiteral("a.tex")));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document), QString());
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document, QStringLiteral("context")), QStringLiteral("context"));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document, QStringLiteral("ConTeXt (LuaMetaTeX)")), QString());
	document.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document), QStringLiteral("context"));
	document.setPlainText(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document, QStringLiteral("context")), QString());
	store(document, createFile(directory, QStringLiteral("a.txt")));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document, QStringLiteral("context")), QString());
}

void LanguageServiceDocumentTest::texIntentChanges()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(document, createFile(directory, QStringLiteral("a.tex")));
	manager.registerDocument(&document, QStringLiteral("context"));
	QVERIFY(service->operations.isEmpty());
	document.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 0);
	QTRY_COMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.last().kind, Operation::Open);
	document.setPlainText(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	const auto operationsBeforeDeferredClose = service->operations.size();
	QVERIFY(operationsBeforeDeferredClose > 1);
	QCOMPARE(service->operations.last().kind, Operation::Change);
	QTRY_COMPARE(service->operations.size(), operationsBeforeDeferredClose + 1);
	QCOMPARE(service->operations.last().kind, Operation::Close);
}

void LanguageServiceDocumentTest::deferredRefreshIsolation()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument first(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	Tw::Document::TeXDocument second(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(first, createFile(directory, QStringLiteral("first.tex")));
	store(second, createFile(directory, QStringLiteral("second.tex")));
	manager.registerDocument(&first, QStringLiteral("context"));
	manager.registerDocument(&second, QStringLiteral("context"));

	first.setPlainText(QStringLiteral("%!TEX program = context\n%!TEX encoding = UTF-8\nbody"));
	QCOMPARE(service->operations.size(), 0);
	first.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 0);
	QTRY_COMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.first().kind, Operation::Open);
	QCOMPARE(service->operations.first().url, manager.bindingForDocument(&first)->url());
	QVERIFY(manager.bindingForDocument(&second)->languageId().isEmpty());

	second.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 1);
	QTRY_COMPARE(service->operations.size(), 2);
	QCOMPARE(service->operations.last().kind, Operation::Open);
	QCOMPARE(service->operations.last().url, manager.bindingForDocument(&second)->url());
}

void LanguageServiceDocumentTest::deferredRefreshLifecycle()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());

	Tw::Document::TeXDocument removed(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(removed, createFile(directory, QStringLiteral("removed.tex")));
	manager.registerDocument(&removed, QStringLiteral("context"));
	removed.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 0);
	manager.unregisterDocument(&removed);

	Tw::Document::TeXDocument survivor(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(survivor, createFile(directory, QStringLiteral("survivor.tex")));
	manager.registerDocument(&survivor, QStringLiteral("context"));
	survivor.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 0);
	QTRY_COMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.first().url, manager.bindingForDocument(&survivor)->url());

	Tw::Document::TeXDocument * destroyed = new Tw::Document::TeXDocument(
	    QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	const QPointer<Tw::Document::TeXDocument> destroyedGuard(destroyed);
	store(*destroyed, createFile(directory, QStringLiteral("destroyed.tex")));
	manager.registerDocument(destroyed, QStringLiteral("context"));
	destroyed->setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 1);
	delete destroyed;
	QVERIFY(destroyedGuard.isNull());

	Tw::Document::TeXDocument trigger(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(trigger, createFile(directory, QStringLiteral("trigger.tex")));
	manager.registerDocument(&trigger, QStringLiteral("context"));
	trigger.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 1);
	QTRY_COMPARE(service->operations.size(), 2);
	QCOMPARE(service->operations.last().url, manager.bindingForDocument(&trigger)->url());

	Tw::Document::TeXDocument managerDocument(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(managerDocument, createFile(directory, QStringLiteral("manager.tex")));
	LanguageServiceManager * destroyedManager = new LanguageServiceManager;
	RecordingService * destroyedService = configure(*destroyedManager, capabilities());
	destroyedManager->registerDocument(&managerDocument, QStringLiteral("context"));
	managerDocument.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(destroyedService->operations.size(), 0);
	delete destroyedManager;

	Tw::Document::TeXDocument finalTrigger(QStringLiteral("%!TEX program = pdfLaTeX\nbody"));
	store(finalTrigger, createFile(directory, QStringLiteral("final-trigger.tex")));
	manager.registerDocument(&finalTrigger, QStringLiteral("context"));
	finalTrigger.setPlainText(QStringLiteral("%!TEX program = context\nbody"));
	QCOMPARE(service->operations.size(), 2);
	QTRY_COMPARE(service->operations.size(), 3);
	QCOMPARE(service->operations.last().url, manager.bindingForDocument(&finalTrigger)->url());
}

void LanguageServiceDocumentTest::unsavedThenStored()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("alpha\n"));
	manager.registerDocument(&document);
	QVERIFY(service->operations.isEmpty());
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.updateDocumentIdentity(&document);
	QCOMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.first().kind, Operation::Open);
	QCOMPARE(service->operations.first().languageId, QStringLiteral("context"));
	QCOMPARE(service->operations.first().version, quint64(1));
	QCOMPARE(service->operations.first().text, document.canonicalText());
}

void LanguageServiceDocumentTest::openCloseCapability()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, false));
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkiv")));
	manager.registerDocument(&document);
	QVERIFY(service->operations.isEmpty());
	QTextCursor cursor(&document);
	cursor.insertText(QStringLiteral("x"));
	QCOMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.first().kind, Operation::Change);
	manager.unregisterDocument(&document);
	QCOMPARE(service->operations.size(), 1);
}

void LanguageServiceDocumentTest::syncNone()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::None));
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	QTextCursor(&document).insertText(QStringLiteral("x"));
	QCOMPARE(service->operations.size(), 1);
	QCOMPARE(service->operations.first().kind, Operation::Open);
}

void LanguageServiceDocumentTest::fullSynchronization()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Full));
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	QTextCursor cursor(&document);
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(QStringLiteral("\né"));
	QCOMPARE(service->operations.size(), 2);
	const Operation change = service->operations.last();
	QCOMPARE(change.kind, Operation::Change);
	QCOMPARE(change.version, quint64(2));
	QVERIFY(!change.hasRange);
	QCOMPARE(change.text, document.canonicalText());
}

void LanguageServiceDocumentTest::incrementalChanges_data()
{
	QTest::addColumn<QString>("initial");
	QTest::addColumn<int>("position");
	QTest::addColumn<int>("removed");
	QTest::addColumn<QString>("inserted");
	QTest::addColumn<int>("startLine");
	QTest::addColumn<int>("startCharacter");
	QTest::addColumn<int>("endLine");
	QTest::addColumn<int>("endCharacter");
	QTest::newRow("ASCII") << QStringLiteral("abc") << 1 << 1 << QStringLiteral("X") << 0 << 1 << 0 << 2;
	QTest::newRow("BMP") << QString::fromUtf8("aébc") << 1 << 1 << QStringLiteral("X") << 0 << 1 << 0 << 2;
	QTest::newRow("supplementary before") << QString::fromUtf8("😀abc") << 3 << 1 << QStringLiteral("X") << 0 << 3 << 0 << 4;
	QTest::newRow("supplementary removed") << QString::fromUtf8("a😀b") << 1 << 2 << QStringLiteral("X") << 0 << 1 << 0 << 3;
	QTest::newRow("multiline insert") << QStringLiteral("ab\ncd") << 3 << 0 << QStringLiteral("X\nY") << 1 << 0 << 1 << 0;
	QTest::newRow("multiline replace") << QStringLiteral("ab\ncd\nef") << 1 << 6 << QStringLiteral("Z") << 0 << 1 << 2 << 1;
}

void LanguageServiceDocumentTest::incrementalChanges()
{
	QFETCH(QString, initial);
	QFETCH(int, position);
	QFETCH(int, removed);
	QFETCH(QString, inserted);
	QFETCH(int, startLine);
	QFETCH(int, startCharacter);
	QFETCH(int, endLine);
	QFETCH(int, endCharacter);
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(initial);
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	QTextCursor cursor(&document);
	cursor.setPosition(position);
	cursor.setPosition(position + removed, QTextCursor::KeepAnchor);
	cursor.insertText(inserted);
	QCOMPARE(service->operations.size(), 2);
	const Operation change = service->operations.last();
	QVERIFY(change.hasRange);
	comparePosition(change.range.start, startLine, startCharacter);
	comparePosition(change.range.end, endLine, endCharacter);
	QCOMPARE(change.text, inserted);
	QCOMPARE(change.version, quint64(2));
	LanguageServiceDocumentBinding * binding = manager.bindingForDocument(&document);
	QVERIFY(binding);
	QCOMPARE(binding->shadow(), document.canonicalText());
}

void LanguageServiceDocumentTest::undoRedo()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	document.setUndoRedoEnabled(true);
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	QTextCursor cursor(&document);
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(QStringLiteral("x"));
	document.undo();
	document.redo();
	QCOMPARE(service->operations.size(), 4);
	QCOMPARE(service->operations.at(1).version, quint64(2));
	QCOMPARE(service->operations.at(2).version, quint64(3));
	QCOMPARE(service->operations.at(3).version, quint64(4));
	QCOMPARE(manager.bindingForDocument(&document)->shadow(), document.canonicalText());
}

void LanguageServiceDocumentTest::sameIdentityIsSilent()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	manager.updateDocumentIdentity(&document);
	QCOMPARE(service->operations.size(), 1);
}

void LanguageServiceDocumentTest::identityAndEligibilityTransitions()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	const QString first = createFile(directory, QStringLiteral("a.mkxl"));
	const QString second = createFile(directory, QStringLiteral("b.mkiv"));
	store(document, first);
	manager.registerDocument(&document);
	store(document, second);
	manager.updateDocumentIdentity(&document);
	QCOMPARE(service->operations.size(), 3);
	QCOMPARE(service->operations.at(1).kind, Operation::Close);
	QCOMPARE(service->operations.at(1).url, QUrl::fromLocalFile(QFileInfo(first).canonicalFilePath()));
	QCOMPARE(service->operations.at(2).kind, Operation::Open);
	QCOMPARE(service->operations.at(2).version, quint64(1));

	store(document, createFile(directory, QStringLiteral("c.txt")));
	manager.updateDocumentIdentity(&document);
	QCOMPARE(service->operations.last().kind, Operation::Close);
	const auto afterClose = service->operations.size();
	QTextCursor(&document).insertText(QStringLiteral("x"));
	QCOMPARE(service->operations.size(), afterClose);

	store(document, createFile(directory, QStringLiteral("d.mkxl")));
	manager.updateDocumentIdentity(&document);
	QCOMPARE(service->operations.last().kind, Operation::Open);
	QCOMPARE(service->operations.last().version, quint64(1));
}

void LanguageServiceDocumentTest::reloadSynchronizes()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("old\ntext"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	document.setPlainText(QString::fromUtf8("new 😀\ntext\nmore"));
	QVERIFY(service->operations.size() >= 2);
	QCOMPARE(manager.bindingForDocument(&document)->shadow(), document.canonicalText());
	QCOMPARE(service->operations.last().version, manager.bindingForDocument(&document)->version());
}

void LanguageServiceDocumentTest::serviceLifecycleAndGeneration()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	const quint64 firstGeneration = manager.bindingForDocument(&document)->serviceGeneration();
	service->fail();
	QVERIFY(!manager.bindingForDocument(&document)->isSynchronized());
	QTextCursor(&document).insertText(QStringLiteral("x"));
	QCOMPARE(service->operations.size(), 1);
	service->stop();
	QVERIFY(service->start());
	QVERIFY(manager.bindingForDocument(&document)->isSynchronized());
	QVERIFY(manager.bindingForDocument(&document)->serviceGeneration() != firstGeneration);
	QCOMPARE(service->operations.last().kind, Operation::Open);
	QCOMPARE(service->operations.last().text, document.canonicalText());
	manager.stop();
	QCOMPARE(service->operations.last().kind, Operation::Close);
	QCOMPARE(service->state(), LanguageService::Stopped);
}

void LanguageServiceDocumentTest::replacementKeepsDocumentBinding()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	LanguageServiceManager manager;
	RecordingService * active = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("replacement.mkxl")));
	manager.registerDocument(&document);
	const quint64 activeGeneration = manager.bindingForDocument(&document)->serviceGeneration();
	active->holdStop = true;

	RecordingService * pending = new RecordingService(capabilities());
	QVERIFY(manager.replaceService(pending, QStringList{QStringLiteral("context")}));
	QVERIFY(manager.bindingForDocument(&document) == nullptr);
	active->finishStop();
	QCOMPARE(manager.service(), static_cast<LanguageService *>(pending));
	QCOMPARE(manager.bindingForDocument(&document)->serviceGeneration(), pending->generation());
	QVERIFY(pending->generation() != activeGeneration);
}

void LanguageServiceDocumentTest::closeLifecycle()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	manager.unregisterDocument(&document);
	QCOMPARE(service->operations.last().kind, Operation::Close);
	QVERIFY(manager.bindingForDocument(&document) == nullptr);

	LanguageServiceManager failedManager;
	RecordingService * failed = configure(failedManager, capabilities());
	Tw::Document::TeXDocument second(QStringLiteral("abc"));
	store(second, createFile(directory, QStringLiteral("b.mkxl")));
	failedManager.registerDocument(&second);
	failed->fail();
	failedManager.unregisterDocument(&second);
	QCOMPARE(failed->operations.size(), 1);
}

void LanguageServiceDocumentTest::multipleDocumentsShareService()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities());
	Tw::Document::TeXDocument first(QStringLiteral("one"));
	Tw::Document::TeXDocument second(QStringLiteral("two"));
	store(first, createFile(directory, QStringLiteral("a.mkxl")));
	store(second, createFile(directory, QStringLiteral("b.mkxl")));
	manager.registerDocument(&first);
	manager.registerDocument(&second);
	QCOMPARE(manager.service(), service);
	QVERIFY(manager.bindingForDocument(&first) != manager.bindingForDocument(&second));
	QTextCursor(&first).insertText(QStringLiteral("x"));
	QTextCursor(&second).insertText(QStringLiteral("y"));
	QCOMPARE(manager.bindingForDocument(&first)->version(), quint64(2));
	QCOMPARE(manager.bindingForDocument(&second)->version(), quint64(2));
	QCOMPARE(manager.bindingForDocument(&first)->shadow(), first.canonicalText());
	QCOMPARE(manager.bindingForDocument(&second)->shadow(), second.canonicalText());
}

void LanguageServiceDocumentTest::completionCapabilityAndRequestContext()
{
	QTemporaryDir directory;
	Tw::Document::TeXDocument unavailableDocument(QStringLiteral("abc"));
	store(unavailableDocument, createFile(directory, QStringLiteral("unavailable.mkxl")));
	LanguageServiceManager unavailableManager;
	RecordingService * unavailable = configure(unavailableManager, capabilities());
	unavailableManager.registerDocument(&unavailableDocument);
	QVERIFY(!unavailableManager.canRequestCompletion(&unavailableDocument));
	QVERIFY(!unavailableManager.requestCompletion(&unavailableDocument, languagePosition(0, 1)));
	QCOMPARE(unavailable->operations.size(), 1);

	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, true, false, true));
	Tw::Document::TeXDocument document(QString::fromUtf8("😀abc"));
	store(document, createFile(directory, QStringLiteral("source.mkxl")));
	manager.registerDocument(&document);
	QVERIFY(manager.canRequestCompletion(&document));
	QSignalSpy readySpy(&manager, SIGNAL(completionReady(Tw::Document::TeXDocument *, const QList<Tw::LanguageServices::CompletionItem> &)));
	QVERIFY(manager.requestCompletion(&document, languagePosition(0, 3)));
	QCOMPARE(service->operations.last().kind, Operation::Completion);
	const LanguageCompletionRequest request = service->operations.last().completionRequest;
	QCOMPARE(request.document, manager.bindingForDocument(&document)->url());
	QCOMPARE(request.synchronizedVersion, quint64(1));
	comparePosition(request.position, 0, 3);
	CompletionItem item;
	item.label = QStringLiteral("provider");
	item.insertText = QStringLiteral("provider");
	service->finishCompletion(request.token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 1);
	QVERIFY(manager.requestCompletion(&document, languagePosition(0, 2)));
	service->failCompletionRequest(service->operations.last().completionRequest.token);
	QCOMPARE(readySpy.count(), 2);
	QVERIFY(qvariant_cast<QList<CompletionItem>>(readySpy.last().at(1)).isEmpty());

	QVERIFY(!manager.requestCompletion(&document, languagePosition(0, 99)));
	service->acceptCompletionRequests = false;
	QVERIFY(!manager.requestCompletion(&document, languagePosition(0, 1)));
}

void LanguageServiceDocumentTest::completionFreshness()
{
	QTemporaryDir directory;
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, true, false, true));
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	store(document, createFile(directory, QStringLiteral("source.mkxl")));
	manager.registerDocument(&document);
	QSignalSpy readySpy(&manager, SIGNAL(completionReady(Tw::Document::TeXDocument *, const QList<Tw::LanguageServices::CompletionItem> &)));
	CompletionItem item;
	item.label = QStringLiteral("provider");
	item.insertText = QStringLiteral("provider");

	const auto requestToken = [&]() {
		if (!manager.requestCompletion(&document, languagePosition(0, 1)))
			return quint64(0);
		return service->operations.last().completionRequest.token;
	};
	quint64 token = requestToken();
	QVERIFY(token != 0);
	QTextCursor(&document).insertText(QStringLiteral("x"));
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 0);

	token = requestToken();
	QVERIFY(token != 0);
	manager.cancelCompletionRequest(&document);
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 0);

	token = requestToken();
	QVERIFY(token != 0);
	manager.beginCompletionEdit(&document);
	QTextCursor cursor(&document);
	cursor.setPosition(0);
	cursor.insertText(QStringLiteral("p"));
	manager.endCompletionEdit(&document);
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 1);

	token = requestToken();
	QVERIFY(token != 0);
	store(document, createFile(directory, QStringLiteral("renamed.mkxl")));
	manager.updateDocumentIdentity(&document);
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 1);

	const quint64 older = requestToken();
	const quint64 newer = requestToken();
	QVERIFY(older != 0 && newer != 0);
	service->finishCompletion(older, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 1);
	service->finishCompletion(newer, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 2);

	token = requestToken();
	QVERIFY(token != 0);
	service->stop();
	QVERIFY(service->start());
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 2);

	Tw::Document::TeXDocument closing(QStringLiteral("close"));
	store(closing, createFile(directory, QStringLiteral("closing.mkxl")));
	manager.registerDocument(&closing);
	QVERIFY(manager.requestCompletion(&closing, languagePosition(0, 1)));
	const quint64 closingToken = service->operations.last().completionRequest.token;
	manager.unregisterDocument(&closing);
	service->finishCompletion(closingToken, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 2);

	token = requestToken();
	QVERIFY(token != 0);
	service->fail();
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 2);
	manager.unregisterDocument(&document);
	service->finishCompletion(token, QList<CompletionItem>{item});
	QCOMPARE(readySpy.count(), 2);
}

void LanguageServiceDocumentTest::definitionCapabilityAndRequestContext()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	Tw::Document::TeXDocument unavailableDocument(QString::fromUtf8("😀abc"));
	store(unavailableDocument, createFile(directory, QStringLiteral("unavailable.mkxl")));
	LanguageServiceManager unavailableManager;
	RecordingService * unavailable = configure(unavailableManager, capabilities());
	unavailableManager.registerDocument(&unavailableDocument);
	QVERIFY(!unavailableManager.canRequestDefinition(&unavailableDocument));
	QVERIFY(!unavailableManager.requestDefinition(&unavailableDocument, languagePosition(0, 2)));
	QCOMPARE(unavailable->operations.size(), 1);

	Tw::Document::TeXDocument document(QString::fromUtf8("😀abc"));
	const QString path = createFile(directory, QStringLiteral("available.mkxl"));
	store(document, path);
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, true, true));
	manager.registerDocument(&document);
	QVERIFY(manager.canRequestDefinition(&document));
	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 3)));
	QCOMPARE(service->operations.size(), 2);
	const LanguageDefinitionRequest request = service->operations.last().definitionRequest;
	QVERIFY(request.token != 0);
	QCOMPARE(request.document, QUrl::fromLocalFile(QFileInfo(path).canonicalFilePath()));
	QCOMPARE(request.synchronizedVersion, quint64(1));
	QCOMPARE(request.position.line, 0);
	QCOMPARE(request.position.character, 3);
	QVERIFY(!manager.requestDefinition(&document, languagePosition(0, 99)));
}

void LanguageServiceDocumentTest::definitionFreshness()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, true, true));
	Tw::Document::TeXDocument document(QStringLiteral("abc\ndef"));
	store(document, createFile(directory, QStringLiteral("a.mkxl")));
	manager.registerDocument(&document);
	int acceptedResults = 0;
	QList<LanguageLocation> acceptedLocations;
	connect(&manager, &LanguageServiceManager::definitionReady, &manager,
	        [&acceptedResults, &acceptedLocations, &document](Tw::Document::TeXDocument * resultDocument,
	                                                        const QList<LanguageLocation> & locations) {
		        if (resultDocument == &document) {
			        ++acceptedResults;
			        acceptedLocations = locations;
		        }
	        });
	LanguageLocation location;
	location.document = QUrl::fromLocalFile(QDir(directory.path()).filePath(QStringLiteral("target.mkxl")));
	location.range.start = languagePosition(1, 0);
	location.range.end = languagePosition(1, 3);
	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	quint64 token = service->operations.last().definitionRequest.token;
	service->finishDefinition(token, QList<LanguageLocation>{});
	QCOMPARE(acceptedResults, 1);
	QVERIFY(acceptedLocations.isEmpty());
	acceptedResults = 0;
	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	service->finishDefinition(token, QList<LanguageLocation>{location, location});
	QCOMPARE(acceptedResults, 1);
	QCOMPARE(acceptedLocations.size(), 2);
	acceptedResults = 0;

	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	QTextCursor(&document).insertText(QStringLiteral("x"));
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 0);

	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	store(document, createFile(directory, QStringLiteral("renamed.mkxl")));
	manager.updateDocumentIdentity(&document);
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 0);

	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	const quint64 superseded = service->operations.last().definitionRequest.token;
	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 2)));
	const quint64 current = service->operations.last().definitionRequest.token;
	service->finishDefinition(superseded, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 0);
	service->finishDefinition(current, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 1);
	QCOMPARE(acceptedLocations.size(), 1);
	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	manager.cancelDefinitionRequest(&document);
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 1);

	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	service->fail();
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 1);
	QVERIFY(!manager.canRequestDefinition(&document));
	service->stop();
	QVERIFY(service->start());
	QVERIFY(manager.canRequestDefinition(&document));
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 1);

	QVERIFY(manager.requestDefinition(&document, languagePosition(0, 1)));
	token = service->operations.last().definitionRequest.token;
	manager.unregisterDocument(&document);
	service->finishDefinition(token, QList<LanguageLocation>{location});
	QCOMPARE(acceptedResults, 1);
}

void LanguageServiceDocumentTest::definitionUnavailableDocuments()
{
	Tw::Document::TeXDocument document(QStringLiteral("abc"));
	LanguageServiceManager noService;
	noService.registerDocument(&document);
	QVERIFY(!noService.canRequestDefinition(&document));
	QVERIFY(!noService.requestDefinition(&document, languagePosition(0, 0)));

	LanguageServiceManager manager;
	RecordingService * service = configure(manager, capabilities(TextSyncKind::Incremental, true, true));
	manager.registerDocument(&document);
	QVERIFY(!manager.canRequestDefinition(&document));
	QVERIFY(!manager.requestDefinition(&document, languagePosition(0, 0)));
	QVERIFY(service->operations.isEmpty());
}

void LanguageServiceDocumentTest::definitionRangeConversion()
{
	QTextDocument document(QString::fromUtf8("a😀b\nsecond"));
	QTextCursor cursor;
	LanguageRange range;
	range.start = languagePosition(0, 1);
	range.end = languagePosition(0, 3);
	QVERIFY(cursorForLanguageRange(&document, range, cursor));
	QCOMPARE(cursor.selectionStart(), 1);
	QCOMPARE(cursor.selectionEnd(), 3);
	QCOMPARE(cursor.selectedText(), QString::fromUtf8("😀"));

	range.start = languagePosition(0, 3);
	range.end = languagePosition(1, 3);
	QVERIFY(cursorForLanguageRange(&document, range, cursor));
	QCOMPARE(cursor.selectionStart(), 3);
	QCOMPARE(cursor.selectionEnd(), 8);

	const QList<LanguageRange> invalidRanges = [&]() {
		QList<LanguageRange> ranges;
		LanguageRange invalid;
		invalid.start = languagePosition(-1, 0);
		invalid.end = languagePosition(0, 0);
		ranges.append(invalid);
		invalid.start = languagePosition(0, 99);
		invalid.end = languagePosition(0, 99);
		ranges.append(invalid);
		invalid.start = languagePosition(9, 0);
		invalid.end = languagePosition(9, 0);
		ranges.append(invalid);
		invalid.start = languagePosition(1, 1);
		invalid.end = languagePosition(0, 1);
		ranges.append(invalid);
		invalid.start = languagePosition(0, 3);
		invalid.end = languagePosition(0, 2);
		ranges.append(invalid);
		return ranges;
	}();
	for (const LanguageRange & invalid : invalidRanges) {
		QTextCursor unchanged(&document);
		unchanged.setPosition(2);
		QVERIFY(!cursorForLanguageRange(&document, invalid, unchanged));
		QCOMPARE(unchanged.position(), 2);
	}
}

QTEST_MAIN(LanguageServiceDocumentTest)
