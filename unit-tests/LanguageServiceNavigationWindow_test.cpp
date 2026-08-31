/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "LanguageServiceNavigationWindow_test.h"

#include "DefaultEngineList.h"
#include "Engine.h"
#include "TWApp.h"
#include "TeXDocumentWindow.h"
#include "languageservices/LanguageService.h"
#include "languageservices/LanguageServiceDocumentBinding.h"
#include "languageservices/LanguageServiceManager.h"
#include "utils/IniConfig.h"
#include "utils/ResourcesLibrary.h"

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QListView>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace Tw::LanguageServices;

namespace {

class RecordingDefinitionService : public LanguageService
{
public:
	bool start() override
	{
		if (!beginStart())
			return false;
		setState(Initializing);
		LanguageServiceCapabilities capabilities;
		capabilities.textSync = TextSyncKind::Incremental;
		capabilities.openClose = true;
		capabilities.completion = true;
		capabilities.definition = true;
		becomeReady(capabilities);
		return true;
	}
	void stop() override
	{
		if (state() == Stopped)
			return;
		setState(Stopping);
		becomeStopped();
	}
	bool openDocument(const LanguageDocumentOpen & document) override
	{
		openedDocuments.append(document);
		providerDocuments.insert(document.url, document.text);
		return true;
	}
	bool changeDocument(const QUrl & url, quint64 version, const LanguageDocumentChange & change) override
	{
		if (!providerDocuments.contains(url))
			return false;
		if (!change.hasRange) {
			providerDocuments.insert(url, change.text);
		}
		else {
			QString & text = providerDocuments[url];
			const auto offset = [&text](const LanguagePosition & position) {
				int line = 0;
				int result = 0;
				while (line < position.line && result < text.size()) {
					const int newline = static_cast<int>(text.indexOf(QLatin1Char('\n'), result));
					if (newline < 0)
						return -1;
					result = newline + 1;
					++line;
				}
				return line == position.line && position.character >= 0
				           && position.character <= text.size() - result ? result + position.character : -1;
			};
			const int start = offset(change.range.start);
			const int end = offset(change.range.end);
			if (start < 0 || end < start)
				return false;
			text.replace(start, end - start, change.text);
		}
		changedVersions.append(version);
		return true;
	}
	bool closeDocument(const QUrl &) override { return true; }
	bool requestCompletion(const LanguageCompletionRequest & request) override
	{
		completionRequests.append(request);
		return true;
	}
	bool requestDefinition(const LanguageDefinitionRequest & request) override
	{
		requests.append(request);
		return true;
	}
	void finish(const QList<LanguageLocation> & locations)
	{
		Q_ASSERT(!requests.isEmpty());
		emit definitionFinished(requests.last().token, locations);
	}
	void finishCompletion(int index, const QList<CompletionItem> & items)
	{
		Q_ASSERT(index >= 0 && index < completionRequests.size());
		emit completionFinished(completionRequests.at(index).token, items);
	}
	void fail() { becomeFailed(QStringLiteral("scripted failure")); }

	QList<LanguageDefinitionRequest> requests;
	QList<LanguageCompletionRequest> completionRequests;
	QList<LanguageDocumentOpen> openedDocuments;
	QList<quint64> changedVersions;
	QHash<QUrl, QString> providerDocuments;
};

QString writeFile(QTemporaryDir & directory, const QString & name, const QString & text)
{
	const QString path = QDir(directory.path()).filePath(name);
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly) || file.write(text.toUtf8()) != text.toUtf8().size())
		return {};
	return path;
}

LanguagePosition position(int line, int character)
{
	LanguagePosition result;
	result.line = line;
	result.character = character;
	return result;
}

LanguageLocation location(const QString & path, int startLine, int startCharacter,
	                      int endLine, int endCharacter)
{
	LanguageLocation result;
	result.document = QUrl::fromLocalFile(path);
	result.range.start = position(startLine, startCharacter);
	result.range.end = position(endLine, endCharacter);
	return result;
}

const Engine * findEngine(const QList<Engine> & engines, const QString & name)
{
	for (const Engine & engine : engines) {
		if (engine.name() == name)
			return &engine;
	}
	return nullptr;
}

class EngineListRestorer
{
public:
	explicit EngineListRestorer(TWApp * application)

		: app(application)
		, originalEngines(application->getEngineList())
	{
	}

	~EngineListRestorer()
	{
		app->setEngineList(originalEngines);
	}

private:
	TWApp * app;
	QList<Engine> originalEngines;
};

} // namespace

void LanguageServiceNavigationWindowTest::engineMetadataPersistence()
{
	EngineListRestorer restoreEngines(TWApp::instance());
	QList<Engine> engines = TWApp::instance()->getEngineList();
	const Engine * luaMetaTeX = findEngine(engines, QStringLiteral("ConTeXt (LuaMetaTeX)"));
	const Engine * luaTeX = findEngine(engines, QStringLiteral("ConTeXt (LuaTeX)"));
	const Engine * historicalPdfTeX = findEngine(engines, QStringLiteral("ConTeXt (pdfTeX)"));
	const Engine * historicalXeTeX = findEngine(engines, QStringLiteral("ConTeXt (XeTeX)"));
	const Engine * pdfLaTeX = findEngine(engines, QStringLiteral("pdfLaTeX"));
	const Engine * lookalike = nullptr;
	for (const Engine & engine : engines) {
		if (engine.name() == QStringLiteral("ConTeXt (LuaMetaTeX)")
		    && engine.program() == QStringLiteral("unrelated")) {
			lookalike = &engine;
			break;
		}
	}
	QVERIFY(luaMetaTeX != nullptr);
	QVERIFY(luaTeX != nullptr);
	QVERIFY(historicalPdfTeX != nullptr);
	QVERIFY(historicalXeTeX != nullptr);
	QVERIFY(pdfLaTeX != nullptr);
	QVERIFY(lookalike != nullptr);
	QCOMPARE(luaMetaTeX->sourceLanguage(), QStringLiteral("context"));
	QCOMPARE(luaTeX->sourceLanguage(), QStringLiteral("context"));
	QCOMPARE(historicalPdfTeX->sourceLanguage(), QStringLiteral("context"));
	QCOMPARE(historicalXeTeX->sourceLanguage(), QStringLiteral("context"));
	QVERIFY(pdfLaTeX->sourceLanguage().isEmpty());
	QVERIFY(lookalike->sourceLanguage().isEmpty());

	const QString toolsPath = QDir(Tw::Utils::ResourcesLibrary::getPortableLibPath())
	                              .filePath(QStringLiteral("configuration/tools.ini"));
	{
		Tw::Utils::IniConfig settings(toolsPath);
		QCOMPARE(settings.value(QStringLiteral("007/language")).toString(), QStringLiteral("context"));
		QVERIFY(!settings.contains(QStringLiteral("014/language")));
	}

	Engine renamed = *luaMetaTeX;
	renamed.setName(QStringLiteral("My renamed tool"));
	QCOMPARE(renamed.sourceLanguage(), QStringLiteral("context"));
	for (int i = 0; i < engines.size(); ++i) {
		if (engines.at(i).program() == luaMetaTeX->program()
		    && engines.at(i).arguments() == luaMetaTeX->arguments()) {
			engines[i] = renamed;
			break;
		}
	}
	TWApp::instance()->setEngineList(engines);
	QCOMPARE(TWApp::instance()->getNamedEngine(renamed.name()).sourceLanguage(), QStringLiteral("context"));

	Tw::Utils::IniConfig settings(toolsPath);
	QCOMPARE(settings.value(QStringLiteral("007/name")).toString(), renamed.name());
	QCOMPARE(settings.value(QStringLiteral("007/language")).toString(), QStringLiteral("context"));
	QVERIFY(!settings.contains(QStringLiteral("014/language")));

	Tw::Document::TeXDocument document;
	document.setFileInfo(QFileInfo(QStringLiteral("renamed.tex")));
	QCOMPARE(LanguageServiceManager::identifyLanguageId(&document, renamed.sourceLanguage()), QStringLiteral("context"));
}

void LanguageServiceNavigationWindowTest::staticCompletionRegression()
{
	TeXDocumentWindow * window = new TeXDocumentWindow;
	window->show();
	CompletingEdit * editor = window->editor();
	editor->setFocus();

	editor->setPlainText(QStringLiteral("adlen"));
	QTextCursor cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab);
	QCOMPARE(editor->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));
	QCOMPARE(editor->textCursor().position(), 13);
	QVERIFY(editor->extraSelections().size() >= 1);

	QTest::keyClick(editor, Qt::Key_Right);
	QTest::qWait(300);
	QCOMPARE(editor->extraSelections().size(), 1);

	editor->setPlainText(QStringLiteral("--"));
	cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\textendash"));
	QTest::keyClick(editor, Qt::Key_Down);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\textendash\\ "));
	QTest::keyClick(editor, Qt::Key_Tab);

	editor->clear();
	QTest::keyClick(editor, Qt::Key_Tab);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\t"));

	editor->setPlainText(QString::fromUtf8("before\u2022after"));
	cursor = editor->textCursor();
	cursor.setPosition(0);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab, Qt::ControlModifier);
	QCOMPARE(editor->textCursor().selectedText(), QString(QChar(0x2022)));

	editor->setPlainText(QStringLiteral("one\ntwo"));
	cursor = editor->textCursor();
	cursor.setPosition(0);
	cursor.setPosition(static_cast<int>(editor->toPlainText().size()), QTextCursor::KeepAnchor);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\tone\n\ttwo"));

	window->setModified(false);
	window->close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
}

void LanguageServiceNavigationWindowTest::completionCandidateList()
{
	TeXDocumentWindow * window = new TeXDocumentWindow;
	window->show();
	CompletingEdit * editor = window->editor();
	editor->setFocus();
	editor->setPlainText(QStringLiteral("\\startsec"));
	QTextCursor cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab);
	auto * list = editor->findChild<QListView *>(QStringLiteral("completionList"));
	QVERIFY(list);
	QVERIFY(list->isVisible());
	QVERIFY(list->model()->rowCount() >= 2);
	QCOMPARE(list->model()->index(0, 0).data().toString(), QStringLiteral("\\startsection"));
	QCOMPARE(list->currentIndex().row(), 0);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsection"));

	QTest::keyClick(editor, Qt::Key_Down);
	QCOMPARE(list->currentIndex().row(), 1);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsectionblock"));
	QTest::keyClick(editor, Qt::Key_Escape);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsec"));
	QVERIFY(!list->isVisible());

	QTest::keyClick(editor, Qt::Key_Tab);
	QTest::keyClick(editor, Qt::Key_Return);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsection"));
	QVERIFY(!list->isVisible());
	window->setModified(false);
	window->close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
}

void LanguageServiceNavigationWindowTest::completionCandidateListDeactivation()
{
	TeXDocumentWindow * window = new TeXDocumentWindow;
	window->show();
	CompletingEdit * editor = window->editor();
	editor->setFocus();
	editor->setPlainText(QStringLiteral("\\startsec"));
	QTextCursor cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	QTest::keyClick(editor, Qt::Key_Tab);
	auto * list = editor->findChild<QListView *>(QStringLiteral("completionList"));
	QVERIFY(list && list->isVisible());
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsection"));
	window->activateWindow();
	QTRY_VERIFY(window->isActiveWindow());

	QWidget otherWindow;
	otherWindow.show();
	otherWindow.raise();
	otherWindow.activateWindow();
	QTRY_VERIFY(!window->isActiveWindow());
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\startsec"));
	QVERIFY(!list->isVisible());

	window->setModified(false);
	window->close();
	otherWindow.close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
}

void LanguageServiceNavigationWindowTest::completionCandidateListProviderIntegration()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString firstPath = writeFile(directory, QStringLiteral("first.mkxl"), QStringLiteral("adlen"));
	const QString secondPath = writeFile(directory, QStringLiteral("second.mkxl"), QStringLiteral("adlen"));
	QVERIFY(!firstPath.isEmpty());
	QVERIFY(!secondPath.isEmpty());

	RecordingDefinitionService * service = new RecordingDefinitionService;
	QVERIFY(TWApp::instance()->languageServiceManager().setService(service, QStringList{QStringLiteral("context")}));
	QVERIFY(TWApp::instance()->languageServiceManager().start());
	TeXDocumentWindow * first = new TeXDocumentWindow(firstPath);
	TeXDocumentWindow * second = new TeXDocumentWindow(secondPath);
	first->show();
	second->show();
	auto openAtEnd = [](CompletingEdit * editor) {
		editor->setFocus();
		QTextCursor cursor = editor->textCursor();
		cursor.movePosition(QTextCursor::End);
		editor->setTextCursor(cursor);
		QTest::keyClick(editor, Qt::Key_Tab);
	};
	auto listFor = [](CompletingEdit * editor) {
		return editor->findChild<QListView *>(QStringLiteral("completionList"));
	};

	CompletingEdit * firstEditor = first->editor();
	CompletingEdit * secondEditor = second->editor();
	openAtEnd(firstEditor);
	QCOMPARE(service->completionRequests.size(), 1);
	QListView * firstList = listFor(firstEditor);
	QVERIFY(firstList && firstList->isVisible());
	const int staticRows = firstList->model()->rowCount();
	QVERIFY(staticRows > 0);
	const quint64 selectedId = firstList->currentIndex().data(Qt::UserRole).toULongLong();
	const QString preview = firstEditor->toPlainText();
	const int caret = firstEditor->textCursor().position();
	const int initialWidth = firstList->width();
	const int staticPresentationWidth = firstList->sizeHintForColumn(0);

	CompletionItem duplicate;
	duplicate.label = QStringLiteral("duplicate static");
	duplicate.insertText = preview;
	CompletionItem provider;
	provider.label = QStringLiteral("provider completion item with an intentionally wider display label");
	provider.insertText = QStringLiteral("providerInserted");
	provider.hasReplacementRange = true;
	provider.replacementRange.start = position(0, 0);
	provider.replacementRange.end = position(0, 5);
	CompletionItem sameLabelA;
	sameLabelA.label = QStringLiteral("same label");
	sameLabelA.insertText = QStringLiteral("sameA");
	CompletionItem sameLabelB = sameLabelA;
	sameLabelB.insertText = QStringLiteral("sameB");
	service->finishCompletion(0, QList<CompletionItem>{duplicate, provider, sameLabelA, sameLabelA, sameLabelB});
	QCOMPARE(firstList->model()->rowCount(), staticRows + 3);
	QVERIFY(firstList->isVisible());
	QCOMPARE(firstList->currentIndex().data(Qt::UserRole).toULongLong(), selectedId);
	QCOMPARE(firstEditor->toPlainText(), preview);
	QCOMPARE(firstEditor->textCursor().position(), caret);
	QCOMPARE(firstList->model()->index(staticRows + 1, 0).data().toString(), QStringLiteral("same label"));
	QCOMPARE(firstList->model()->index(staticRows + 2, 0).data().toString(), QStringLiteral("same label"));

	const QModelIndex providerIndex = firstList->model()->index(staticRows, 0);
	QCOMPARE(providerIndex.data().toString(), provider.label);
	const int providerWidth = firstList->sizeHintForIndex(providerIndex).width();
	QVERIFY(providerWidth > staticPresentationWidth);
	QVERIFY(firstList->width() > initialWidth);
	QVERIFY(firstList->viewport()->width() >= providerWidth);
	QTest::mouseClick(firstList->viewport(), Qt::LeftButton, Qt::NoModifier, firstList->visualRect(providerIndex).center());
	QCOMPARE(firstEditor->toPlainText(), QStringLiteral("providerInserted"));
	const QString accepted = firstEditor->toPlainText();
	QTest::keyClick(firstEditor, Qt::Key_Tab);
	QCOMPARE(firstEditor->toPlainText(), accepted);
	QVERIFY(!firstList->isVisible());

	firstEditor->setPlainText(QStringLiteral("adlen"));
	openAtEnd(firstEditor);
	const int invalidRequest = static_cast<int>(service->completionRequests.size()) - 1;
	QVERIFY(firstList->isVisible());
	const int rowsBeforeInvalid = firstList->model()->rowCount();
	CompletionItem invalid = provider;
	invalid.replacementRange.start = position(1, 0);
	invalid.replacementRange.end = position(1, 1);
	service->finishCompletion(invalidRequest, QList<CompletionItem>{invalid});
	QCOMPARE(firstList->model()->rowCount(), rowsBeforeInvalid);
	QVERIFY(firstList->isVisible());

	secondEditor->setPlainText(QStringLiteral("adlen"));
	openAtEnd(secondEditor);
	QListView * secondList = listFor(secondEditor);
	QVERIFY(secondList && secondList->isVisible());
	const int secondRows = secondList->model()->rowCount();
	const quint64 secondId = secondList->currentIndex().data(Qt::UserRole).toULongLong();
	const QString secondPreview = secondEditor->toPlainText();
	firstEditor->setPlainText(QStringLiteral("adlen"));
	openAtEnd(firstEditor);
	const int appendRequest = static_cast<int>(service->completionRequests.size()) - 1;
	service->finishCompletion(appendRequest, QList<CompletionItem>{sameLabelA});
	QCOMPARE(firstList->model()->rowCount(), staticRows + 1);
	QCOMPARE(secondList->model()->rowCount(), secondRows);
	QCOMPARE(secondList->currentIndex().data(Qt::UserRole).toULongLong(), secondId);
	QCOMPARE(secondEditor->toPlainText(), secondPreview);

	firstEditor->setPlainText(QStringLiteral("providerPrefix"));
	openAtEnd(firstEditor);
	const int escapeRequest = static_cast<int>(service->completionRequests.size()) - 1;
	QTest::keyClick(firstEditor, Qt::Key_Escape);
	service->finishCompletion(escapeRequest, QList<CompletionItem>{sameLabelA});
	QCOMPARE(firstEditor->toPlainText(), QStringLiteral("providerPrefix"));
	QVERIFY(!firstList->isVisible());

	firstEditor->setPlainText(QStringLiteral("adlen"));
	openAtEnd(firstEditor);
	const int oldRequest = static_cast<int>(service->completionRequests.size()) - 1;
	QTest::keyClick(firstEditor, Qt::Key_X);
	QCOMPARE(firstEditor->toPlainText(), QStringLiteral("adlenx"));
	QCOMPARE(service->completionRequests.size(), oldRequest + 2);
	service->finishCompletion(oldRequest, QList<CompletionItem>{sameLabelA});
	QCOMPARE(firstEditor->toPlainText(), QStringLiteral("adlenx"));
	service->finishCompletion(oldRequest + 1, QList<CompletionItem>{sameLabelB});
	QCOMPARE(firstEditor->toPlainText(), QStringLiteral("sameB"));
	QTest::keyClick(firstEditor, Qt::Key_Escape);

	firstEditor->setPlainText(QStringLiteral("adlen"));
	openAtEnd(firstEditor);
	const int backspaceRequest = static_cast<int>(service->completionRequests.size()) - 1;
	QTest::keyClick(firstEditor, Qt::Key_Backspace);
	QCOMPARE(firstEditor->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));
	QCOMPARE(service->completionRequests.size(), backspaceRequest + 2);
	service->finishCompletion(backspaceRequest, QList<CompletionItem>{sameLabelA});
	QCOMPARE(firstEditor->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));
	QTest::keyClick(firstEditor, Qt::Key_Escape);

	service->fail();
	QVERIFY(secondList->isVisible());
	QTest::keyClick(secondEditor, Qt::Key_Tab);
	QVERIFY(!secondList->isVisible());
	QCOMPARE(secondEditor->toPlainText(), secondPreview);
	second->setModified(false);
	second->close();
	first->setModified(false);
	first->close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
	QVERIFY(TWApp::instance()->languageServiceManager().replaceService(nullptr));
	QCOMPARE(TWApp::instance()->languageServiceManager().state(), LanguageService::NotConfigured);
}

void LanguageServiceNavigationWindowTest::completionPreviewUndoRedoSynchronization()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString sourcePath = writeFile(directory, QStringLiteral("preview-undo.mkxl"), QStringLiteral("--"));
	TeXDocumentWindow * window = new TeXDocumentWindow(sourcePath);
	window->show();
	window->selectWindow();

	auto * service = new RecordingDefinitionService;
	QVERIFY(TWApp::instance()->languageServiceManager().setService(service, QStringList{QStringLiteral("context")}));
	QVERIFY(TWApp::instance()->languageServiceManager().start());
	LanguageServiceDocumentBinding * binding = TWApp::instance()->languageServiceManager().bindingForDocument(window->textDoc());
	QVERIFY(binding);
	QTRY_VERIFY(binding->isSynchronized());

	const auto verifyState = [&]() {
		QCOMPARE(window->textDoc()->canonicalText(), window->editor()->toPlainText());
		QCOMPARE(binding->shadow(), window->textDoc()->canonicalText());
		QCOMPARE(service->providerDocuments.value(QUrl::fromLocalFile(sourcePath)), window->textDoc()->canonicalText());
	};
	CompletingEdit * editor = window->editor();
	editor->setFocus();
	QTextCursor cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	verifyState();
	const quint64 baseVersion = binding->version();

	QTest::keyClick(editor, Qt::Key_Tab);
	QCOMPARE(editor->toPlainText(), QStringLiteral("\\textendash"));
	QCOMPARE(service->completionRequests.size(), 1);
	verifyState();
	QVERIFY(binding->version() > baseVersion);
	const quint64 firstPreviewVersion = binding->version();

	QTest::keyClick(editor, Qt::Key_Down);
	const QString cycledPreview = editor->toPlainText();
	QVERIFY(cycledPreview != QStringLiteral("--"));
	QVERIFY(cycledPreview != QStringLiteral("\\textendash"));
	verifyState();
	QVERIFY(binding->version() > firstPreviewVersion);
	quint64 restoredVersion = binding->version();
	QTest::keyClick(editor, Qt::Key_Escape);
	verifyState();
	QVERIFY(binding->version() > restoredVersion);
	QCOMPARE(editor->toPlainText(), QStringLiteral("--"));

	QTest::keyClick(editor, Qt::Key_Z, Qt::ControlModifier);
	QTRY_VERIFY(binding->version() > restoredVersion);
	verifyState();
	const QString undoText = editor->toPlainText();
	QCOMPARE(undoText, cycledPreview);

	CompletionItem late;
	late.label = QStringLiteral("late");
	late.insertText = QStringLiteral("late");
	service->finishCompletion(0, QList<CompletionItem>{late});
	QCOMPARE(editor->toPlainText(), undoText);
	verifyState();

	const quint64 undoVersion = binding->version();
	QTest::keyClick(editor, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
	QTRY_VERIFY(binding->version() > undoVersion);
	verifyState();
	QCOMPARE(editor->toPlainText(), QStringLiteral("--"));

	for (int index = 1; index < service->changedVersions.size(); ++index)
		QVERIFY(service->changedVersions.at(index) > service->changedVersions.at(index - 1));
	QVERIFY(TWApp::instance()->languageServiceManager().replaceService(nullptr, {}));
	QTRY_VERIFY(TWApp::instance()->languageServiceManager().service() == nullptr);
	window->setModified(false);
	window->close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
}

void LanguageServiceNavigationWindowTest::actionAndNavigation()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString sourcePath = writeFile(directory, QStringLiteral("source.mkxl"), QString::fromUtf8("a😀b\nsource\n"));
	const QString targetPath = writeFile(directory, QStringLiteral("target.mkxl"), QStringLiteral("first\ndefinition\n"));
	QVERIFY(!sourcePath.isEmpty());
	QVERIFY(!targetPath.isEmpty());

	TeXDocumentWindow * source = new TeXDocumentWindow(sourcePath);
	source->show();
	QAction * action = source->findChild<QAction *>(QStringLiteral("actionGo_to_Definition"));
	QVERIFY(action);
	QVERIFY(!action->isEnabled());

	RecordingDefinitionService * service = new RecordingDefinitionService;
	QVERIFY(TWApp::instance()->languageServiceManager().setService(service, QStringList{QStringLiteral("context")}));
	QVERIFY(TWApp::instance()->languageServiceManager().start());
	QTRY_VERIFY(action->isEnabled());

	const QString intentPath = writeFile(directory, QStringLiteral("intent.tex"), QStringLiteral("plain TeX source\n"));
	QVERIFY(!intentPath.isEmpty());
	const auto openedBeforeIntent = service->openedDocuments.size();
	const QList<Engine> engines = TWApp::instance()->getEngineList();
	const Engine * luaMetaTeX = findEngine(engines, QStringLiteral("ConTeXt (LuaMetaTeX)"));
	QVERIFY(luaMetaTeX != nullptr);
	QCOMPARE(luaMetaTeX->sourceLanguage(), QStringLiteral("context"));
	TeXDocumentWindow * intentWindow = new TeXDocumentWindow(intentPath);
	intentWindow->show();
	QComboBox * engineSelector = nullptr;
	for (QComboBox * candidate : intentWindow->findChildren<QComboBox *>()) {
		if (candidate->findText(luaMetaTeX->name(), Qt::MatchFixedString) >= 0) {
			engineSelector = candidate;
			break;
		}
	}
	QVERIFY(engineSelector != nullptr);
	engineSelector->setCurrentIndex(engineSelector->findText(luaMetaTeX->name(), Qt::MatchFixedString));
	QTRY_COMPARE(service->openedDocuments.size(), openedBeforeIntent + 1);
	QCOMPARE(service->openedDocuments.last().languageId, QStringLiteral("context"));
	QCOMPARE(service->openedDocuments.last().url, QUrl::fromLocalFile(intentPath));
	intentWindow->close();
	QTRY_COMPARE(TeXDocumentWindow::documentList().size(), 1);
	const auto placeAtEnd = [](CompletingEdit * editor) {
		QTextCursor cursor = editor->textCursor();
		cursor.movePosition(QTextCursor::End);
		editor->setTextCursor(cursor);
		editor->setFocus();
	};

	source->editor()->setPlainText(QStringLiteral("adlen"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(service->completionRequests.size(), 1);
	QCOMPARE(source->editor()->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));
	CompletionItem duplicate;
	duplicate.label = QStringLiteral("adlen");
	duplicate.insertText = QString::fromUtf8("\\addtolength{}{\u2022}\n");
	service->finishCompletion(0, QList<CompletionItem>{duplicate});
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(source->editor()->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));

	source->editor()->setPlainText(QStringLiteral("providerPrefix"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(service->completionRequests.size(), 2);
	QCOMPARE(source->editor()->toPlainText(), QStringLiteral("providerPrefix"));
	CompletionItem provider;
	provider.label = QStringLiteral("providerLabel");
	provider.insertText = QStringLiteral("providerInserted");
	service->finishCompletion(1, QList<CompletionItem>{provider});
	QCOMPARE(source->editor()->toPlainText(), QStringLiteral("providerInserted"));

	source->editor()->setPlainText(QString::fromUtf8("😀pr"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(service->completionRequests.size(), 3);
	QCOMPARE(service->completionRequests.last().position.character, 4);
	CompletionItem edit;
	edit.label = QStringLiteral("providerEdit");
	edit.insertText = QStringLiteral("providerEdit");
	edit.hasReplacementRange = true;
	edit.replacementRange.start = position(0, 2);
	edit.replacementRange.end = position(0, 4);
	service->finishCompletion(2, QList<CompletionItem>{edit});
	QCOMPARE(source->editor()->toPlainText(), QString::fromUtf8("😀providerEdit"));

	source->editor()->setPlainText(QStringLiteral("lateProvider"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(service->completionRequests.size(), 4);
	QTest::keyClick(source->editor(), Qt::Key_Left);
	service->finishCompletion(3, QList<CompletionItem>{provider});
	QCOMPARE(source->editor()->toPlainText(), QStringLiteral("lateProvider"));

	source->editor()->setPlainText(QStringLiteral("invalidRange"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	CompletionItem invalid = edit;
	invalid.replacementRange.start = position(1, 0);
	invalid.replacementRange.end = position(1, 1);
	service->finishCompletion(4, QList<CompletionItem>{invalid});
	QCOMPARE(source->editor()->toPlainText(), QStringLiteral("invalidRange"));

	TeXDocumentWindow * isolated = new TeXDocumentWindow(targetPath);
	isolated->show();
	source->editor()->setPlainText(QStringLiteral("sourceOnly"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	isolated->editor()->setPlainText(QStringLiteral("targetOnly"));
	placeAtEnd(isolated->editor());
	QTest::keyClick(isolated->editor(), Qt::Key_Tab);
	QCOMPARE(service->completionRequests.size(), 7);
	service->finishCompletion(5, QList<CompletionItem>{provider});
	QCOMPARE(source->editor()->toPlainText(), QStringLiteral("providerInserted"));
	QCOMPARE(isolated->editor()->toPlainText(), QStringLiteral("targetOnly"));
	CompletionItem targetProvider = provider;
	targetProvider.insertText = QStringLiteral("targetInserted");
	service->finishCompletion(6, QList<CompletionItem>{targetProvider});
	QCOMPARE(isolated->editor()->toPlainText(), QStringLiteral("targetInserted"));
	isolated->setModified(false);
	isolated->close();
	QTRY_COMPARE(TeXDocumentWindow::documentList().size(), 1);

	source->editor()->setPlainText(QString::fromUtf8("a😀b\nsource\n"));
	QTextCursor requestCursor = source->textCursor();
	requestCursor.setPosition(3);
	source->editor()->setTextCursor(requestCursor);
	action->trigger();
	QCOMPARE(service->requests.size(), 1);
	QCOMPARE(service->requests.last().position.line, 0);
	QCOMPARE(service->requests.last().position.character, 3);

	service->finish(QList<LanguageLocation>{location(sourcePath, 0, 1, 0, 3)});
	QCOMPARE(TeXDocumentWindow::documentList().size(), 1);
	QCOMPARE(source->textCursor().selectedText(), QString::fromUtf8("😀"));

	action->trigger();
	service->finish(QList<LanguageLocation>{location(targetPath, 1, 0, 1, 10)});
	TeXDocumentWindow * target = TeXDocumentWindow::findDocument(targetPath);
	QVERIFY(target);
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);
	QCOMPARE(target->textCursor().selectedText(), QStringLiteral("definition"));

	source->selectWindow();
	action->trigger();
	service->finish(QList<LanguageLocation>{location(targetPath, 1, 0, 1, 3)});
	QCOMPARE(TeXDocumentWindow::findDocument(targetPath), target);
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);
	QCOMPARE(target->textCursor().selectedText(), QStringLiteral("def"));

	source->selectWindow();
	action->trigger();
	service->finish(QList<LanguageLocation>{location(targetPath, 1, 99, 1, 100)});
	QCOMPARE(target->textCursor().selectedText(), QStringLiteral("def"));
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);

	const QString unopenedInvalidTargetPath = writeFile(directory, QStringLiteral("unopened-invalid-target.mkxl"), QStringLiteral("target\n"));
	QVERIFY(!unopenedInvalidTargetPath.isEmpty());
	QVERIFY(TeXDocumentWindow::findDocument(unopenedInvalidTargetPath) == nullptr);
	const auto documentsBeforeInvalidUnopenedTarget = TeXDocumentWindow::documentList().size();
	const QTextCursor sourceCursorBeforeInvalidUnopenedTarget = source->textCursor();
	source->selectWindow();
	action->trigger();
	service->finish(QList<LanguageLocation>{location(unopenedInvalidTargetPath, 0, 99, 0, 100)});
	QVERIFY(TeXDocumentWindow::findDocument(unopenedInvalidTargetPath) == nullptr);
	QCOMPARE(TeXDocumentWindow::documentList().size(), documentsBeforeInvalidUnopenedTarget);
	QCOMPARE(source->textCursor().position(), sourceCursorBeforeInvalidUnopenedTarget.position());
	QCOMPARE(source->textCursor().anchor(), sourceCursorBeforeInvalidUnopenedTarget.anchor());
	QCOMPARE(source->statusBar()->currentMessage(), QStringLiteral("Cannot open definition location"));

	action->trigger();
	LanguageLocation nonLocal = location(targetPath, 0, 0, 0, 1);
	nonLocal.document = QUrl(QStringLiteral("untitled:definition"));
	service->finish(QList<LanguageLocation>{nonLocal});
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);

	action->trigger();
	service->finish(QList<LanguageLocation>{});
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);
	action->trigger();
	service->finish(QList<LanguageLocation>{location(sourcePath, 0, 0, 0, 1),
	                                               location(targetPath, 0, 0, 0, 1)});
	QCOMPARE(TeXDocumentWindow::documentList().size(), 2);

	service->fail();
	QTRY_VERIFY(!action->isEnabled());
	source->editor()->setPlainText(QStringLiteral("adlen"));
	placeAtEnd(source->editor());
	QTest::keyClick(source->editor(), Qt::Key_Tab);
	QCOMPARE(source->editor()->toPlainText(), QString::fromUtf8("\\addtolength{}{\u2022}\n"));
	target->close();
	source->setModified(false);
	source->close();
	QTRY_VERIFY(TeXDocumentWindow::documentList().isEmpty());
}

int main(int argc, char * argv[])
{
	QStandardPaths::setTestModeEnabled(true);
	QTemporaryDir library;
	if (!library.isValid())
		return 1;
	qputenv("TW_INIPATH", QFile::encodeName(library.path()));
	Tw::Utils::ResourcesLibrary::setPortableLibPath(library.path());
	QDir configuration(QDir(library.path()).filePath(QStringLiteral("configuration")));
	if (!configuration.mkpath(QStringLiteral(".")))
		return 1;
	{
		Tw::Utils::IniConfig settings(configuration.filePath(QStringLiteral("tools.ini")));
		int index = 0;
		for (const Tw::DefaultEngineList::Definition & definition : Tw::DefaultEngineList::definitions()) {
			settings.beginGroup(QStringLiteral("%1").arg(++index, 3, 10, QChar::fromLatin1('0')));
			settings.setValue(QStringLiteral("name"), definition.name);
			settings.setValue(QStringLiteral("program"), definition.program);
			settings.setValue(QStringLiteral("arguments"), definition.arguments);
			settings.setValue(QStringLiteral("showPdf"), definition.showPdf);
			settings.endGroup();
		}
		settings.beginGroup(QStringLiteral("014"));
		settings.setValue(QStringLiteral("name"), QStringLiteral("ConTeXt (LuaMetaTeX)"));
		settings.setValue(QStringLiteral("program"), QStringLiteral("unrelated"));
		settings.setValue(QStringLiteral("arguments"), QStringList{QStringLiteral("--custom")});
		settings.setValue(QStringLiteral("showPdf"), false);
		settings.endGroup();
	}
	int applicationArgc = 1;
	char * applicationArgv[] = {argv[0], nullptr};
	TWApp application(applicationArgc, applicationArgv);
	LanguageServiceNavigationWindowTest test;
	return QTest::qExec(&test, argc, argv);
}
