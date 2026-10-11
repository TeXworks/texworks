/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "LanguageServiceSession_test.h"

#include "languageservices/LanguageServiceManager.h"
#include "languageservices/lsp/LspLanguageService.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Tw::LanguageServices::LanguageService;
using Tw::LanguageServices::LanguageServiceCapabilities;
using Tw::LanguageServices::LanguageServiceConfiguration;
using Tw::LanguageServices::LanguageDocumentChange;
using Tw::LanguageServices::LanguageDocumentOpen;
using Tw::LanguageServices::LanguageCompletionRequest;
using Tw::LanguageServices::CompletionItem;
using Tw::LanguageServices::LanguageDefinitionRequest;
using Tw::LanguageServices::LanguageLocation;
using Tw::LanguageServices::LanguageServiceManager;
using Tw::LanguageServices::TextSyncKind;
using Tw::LanguageServices::Lsp::LspLanguageService;

namespace {

QString fakeServerPath()
{
	QString name = QStringLiteral("language_server_fake");
#ifdef Q_OS_WIN
	name += QStringLiteral(".exe");
#endif
	return QDir(QCoreApplication::applicationDirPath()).filePath(name);
}

LanguageServiceConfiguration configuration(const QString & profile)
{
	LanguageServiceConfiguration result;
	result.executable = fakeServerPath();
	result.arguments = {QStringLiteral("--lsp-profile"), profile};
	return result;
}

void waitForReady(LspLanguageService & service)
{
	QVERIFY(service.start());
	QTRY_COMPARE(service.state(), LanguageService::Ready);
}

void stopAndWait(LspLanguageService & service)
{
	service.stop();
	QTRY_COMPARE(service.state(), LanguageService::Stopped);
}

bool capabilitiesAreEmpty(const LanguageServiceCapabilities & capabilities)
{
	return capabilities == LanguageServiceCapabilities{};
}

class ControlledStopService : public LanguageService
{
public:
	bool start() override
	{
		if (!beginStart())
			return false;
		setState(Initializing);
		becomeReady(LanguageServiceCapabilities{});
		return true;
	}
	void stop() override
	{
		if (state() != Stopped)
			setState(Stopping);
	}
	void finishStop() { becomeStopped(); }
	bool openDocument(const LanguageDocumentOpen &) override { return true; }
	bool changeDocument(const QUrl &, quint64, const LanguageDocumentChange &) override { return true; }
	bool closeDocument(const QUrl &) override { return true; }
	bool requestCompletion(const LanguageCompletionRequest &) override { return false; }
	bool requestDefinition(const LanguageDefinitionRequest &) override { return false; }
};

} // namespace

void LanguageServiceSessionTest::initTestCase()
{
	qRegisterMetaType<LanguageService::State>("Tw::LanguageServices::LanguageService::State");
	qRegisterMetaType<LanguageServiceCapabilities>("Tw::LanguageServices::LanguageServiceCapabilities");
	qRegisterMetaType<TextSyncKind>("Tw::LanguageServices::TextSyncKind");
	qRegisterMetaType<QList<LanguageLocation>>("QList<Tw::LanguageServices::LanguageLocation>");
	qRegisterMetaType<QList<CompletionItem>>("QList<Tw::LanguageServices::CompletionItem>");
	QVERIFY2(QFileInfo::exists(fakeServerPath()), qPrintable(fakeServerPath()));
}

void LanguageServiceSessionTest::digestifLikeInitialize()
{
	LspLanguageService service(configuration(QStringLiteral("digestif")), nullptr, 1000, 100, 100);
	QSignalSpy readinessSpy(&service, SIGNAL(readinessChanged(bool)));
	QSignalSpy capabilitiesSpy(&service, SIGNAL(capabilitiesChanged(const Tw::LanguageServices::LanguageServiceCapabilities &)));
	waitForReady(service);
	const LanguageServiceCapabilities capabilities = service.capabilities();
	QCOMPARE(capabilities.textSync, TextSyncKind::Incremental);
	QVERIFY(capabilities.openClose);
	QVERIFY(capabilities.completion);
	QVERIFY(capabilities.signatureHelp);
	QVERIFY(capabilities.hover);
	QVERIFY(capabilities.definition);
	QVERIFY(capabilities.references);
	QVERIFY(capabilities.documentSymbols);
	QVERIFY(capabilities.workspaceSymbols);
	QVERIFY(!capabilities.diagnostics);
	QCOMPARE(readinessSpy.count(), 1);
	QVERIFY(capabilitiesSpy.count() >= 1);
	stopAndWait(service);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::synchronizationMapping_data()
{
	QTest::addColumn<QString>("profile");
	QTest::addColumn<TextSyncKind>("expected");
	QTest::newRow("numeric incremental") << QStringLiteral("numeric-incremental") << TextSyncKind::Incremental;
	QTest::newRow("object incremental") << QStringLiteral("digestif") << TextSyncKind::Incremental;
	QTest::newRow("full") << QStringLiteral("full") << TextSyncKind::Full;
	QTest::newRow("none") << QStringLiteral("none") << TextSyncKind::None;
	QTest::newRow("absent") << QStringLiteral("omitted") << TextSyncKind::None;
}

void LanguageServiceSessionTest::synchronizationMapping()
{
	QFETCH(QString, profile);
	QFETCH(TextSyncKind, expected);
	LspLanguageService service(configuration(profile), nullptr, 1000, 100, 100);
	waitForReady(service);
	QCOMPARE(service.capabilities().textSync, expected);
	stopAndWait(service);
}

void LanguageServiceSessionTest::openCloseMapping_data()
{
	QTest::addColumn<QString>("profile");
	QTest::addColumn<bool>("expected");
	QTest::newRow("true") << QStringLiteral("digestif") << true;
	QTest::newRow("false") << QStringLiteral("open-close-false") << false;
	QTest::newRow("absent") << QStringLiteral("open-close-absent") << false;
}

void LanguageServiceSessionTest::openCloseMapping()
{
	QFETCH(QString, profile);
	QFETCH(bool, expected);
	LspLanguageService service(configuration(profile), nullptr, 1000, 100, 100);
	waitForReady(service);
	QCOMPARE(service.capabilities().openClose, expected);
	stopAndWait(service);
}

void LanguageServiceSessionTest::featureMapping()
{
	LspLanguageService present(configuration(QStringLiteral("digestif")), nullptr, 1000, 100, 100);
	waitForReady(present);
	const LanguageServiceCapabilities enabled = present.capabilities();
	QVERIFY(enabled.completion && enabled.signatureHelp && enabled.hover && enabled.definition);
	QVERIFY(enabled.references && enabled.documentSymbols && enabled.workspaceSymbols);
	stopAndWait(present);

	for (const QString & profile : {QStringLiteral("features-false"), QStringLiteral("omitted")}) {
		LspLanguageService absent(configuration(profile), nullptr, 1000, 100, 100);
		waitForReady(absent);
		const LanguageServiceCapabilities disabled = absent.capabilities();
		QVERIFY(!disabled.completion && !disabled.signatureHelp && !disabled.hover && !disabled.definition);
		QVERIFY(!disabled.references && !disabled.documentSymbols && !disabled.workspaceSymbols && !disabled.diagnostics);
		stopAndWait(absent);
	}
}

void LanguageServiceSessionTest::completionRequestMapping()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString logPath = QDir(directory.path()).filePath(QStringLiteral("completion.jsonl"));
	LanguageServiceConfiguration config = configuration(QStringLiteral("digestif"));
	config.arguments << QStringLiteral("--document-log") << logPath
	                 << QStringLiteral("--completion-mode") << QStringLiteral("label");
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);
	QSignalSpy resultSpy(&service, SIGNAL(completionFinished(quint64, const QList<Tw::LanguageServices::CompletionItem> &)));
	LanguageCompletionRequest request;
	request.token = 41;
	request.document = QUrl::fromLocalFile(QDir(directory.path()).filePath(QStringLiteral("space name.mkxl")));
	request.synchronizedVersion = 7;
	request.position.line = 2;
	request.position.character = 3;
	QVERIFY(service.requestCompletion(request));
	QTRY_COMPARE(resultSpy.count(), 1);
	QCOMPARE(resultSpy.first().at(0).toULongLong(), quint64(41));
	QFile log(logPath);
	QVERIFY(log.open(QIODevice::ReadOnly));
	const QJsonObject message = QJsonDocument::fromJson(log.readLine().trimmed()).object();
	QCOMPARE(message.value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/completion"));
	const QJsonObject params = message.value(QStringLiteral("params")).toObject();
	QCOMPARE(params.value(QStringLiteral("textDocument")).toObject().value(QStringLiteral("uri")).toString(),
	         request.document.toString(QUrl::FullyEncoded));
	QCOMPARE(params.value(QStringLiteral("position")).toObject().value(QStringLiteral("line")).toInt(), 2);
	QCOMPARE(params.value(QStringLiteral("position")).toObject().value(QStringLiteral("character")).toInt(), 3);
	stopAndWait(service);
}

void LanguageServiceSessionTest::completionResults_data()
{
	QTest::addColumn<QString>("mode");
	QTest::addColumn<int>("itemCount");
	QTest::addColumn<bool>("fails");
	QTest::newRow("null") << QStringLiteral("null") << 0 << false;
	QTest::newRow("item array") << QStringLiteral("array") << 2 << false;
	QTest::newRow("CompletionList") << QStringLiteral("list") << 2 << false;
	QTest::newRow("label only") << QStringLiteral("label") << 1 << false;
	QTest::newRow("insertText") << QStringLiteral("insert") << 1 << false;
	QTest::newRow("TextEdit") << QStringLiteral("textedit") << 1 << false;
	QTest::newRow("metadata") << QStringLiteral("metadata") << 1 << false;
	QTest::newRow("delayed") << QStringLiteral("delayed") << 2 << false;
	QTest::newRow("snippet excluded") << QStringLiteral("snippet") << 0 << false;
	QTest::newRow("malformed item skipped") << QStringLiteral("malformed-item") << 0 << false;
	QTest::newRow("invalid range skipped") << QStringLiteral("invalid-range") << 0 << false;
	QTest::newRow("malformed result") << QStringLiteral("malformed") << 0 << true;
	QTest::newRow("provider error") << QStringLiteral("error") << 0 << true;
}

void LanguageServiceSessionTest::completionResults()
{
	QFETCH(QString, mode);
	QFETCH(int, itemCount);
	QFETCH(bool, fails);
	LanguageServiceConfiguration config = configuration(QStringLiteral("digestif"));
	config.arguments << QStringLiteral("--completion-mode") << mode;
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);
	QSignalSpy resultSpy(&service, SIGNAL(completionFinished(quint64, const QList<Tw::LanguageServices::CompletionItem> &)));
	QSignalSpy failureSpy(&service, SIGNAL(completionFailed(quint64, const QString &)));
	LanguageCompletionRequest request;
	request.token = 8;
	request.document = QUrl::fromLocalFile(QStringLiteral("/tmp/source.mkxl"));
	request.position.line = 0;
	request.position.character = 2;
	QVERIFY(service.requestCompletion(request));
	if (fails) {
		QTRY_COMPARE(failureSpy.count(), 1);
		QCOMPARE(resultSpy.count(), 0);
	}
	else {
		QTRY_COMPARE(resultSpy.count(), 1);
		QCOMPARE(failureSpy.count(), 0);
		const QList<CompletionItem> items = qvariant_cast<QList<CompletionItem>>(resultSpy.first().at(1));
		QCOMPARE(items.size(), itemCount);
		if (mode == QStringLiteral("label"))
			QCOMPARE(items.first().insertText, items.first().label);
		if (mode == QStringLiteral("insert"))
			QCOMPARE(items.first().insertText, QStringLiteral("providerInsert"));
		if (mode == QStringLiteral("textedit")) {
			QVERIFY(items.first().hasReplacementRange);
			QCOMPARE(items.first().replacementRange.start.character, 0);
			QCOMPARE(items.first().replacementRange.end.character, 2);
		}
		if (mode == QStringLiteral("metadata")) {
			QCOMPARE(items.first().detail, QStringLiteral("detail"));
			QCOMPARE(items.first().documentation, QStringLiteral("documentation"));
		}
	}
	stopAndWait(service);
}

void LanguageServiceSessionTest::definitionRequestMapping()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString logPath = QDir(directory.path()).filePath(QStringLiteral("definition.jsonl"));
	LanguageServiceConfiguration config = configuration(QStringLiteral("digestif"));
	config.arguments << QStringLiteral("--document-log") << logPath
	                 << QStringLiteral("--definition-mode") << QStringLiteral("location");
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);
	QSignalSpy resultSpy(&service, SIGNAL(definitionFinished(quint64, const QList<Tw::LanguageServices::LanguageLocation> &)));
	LanguageDefinitionRequest request;
	request.token = 42;
	request.document = QUrl::fromLocalFile(QDir(directory.path()).filePath(QStringLiteral("source.mkxl")));
	request.synchronizedVersion = 7;
	request.position.line = 2;
	request.position.character = 3;
	QVERIFY(service.requestDefinition(request));
	QTRY_COMPARE(resultSpy.count(), 1);
	QCOMPARE(resultSpy.first().at(0).toULongLong(), quint64(42));
	const QList<LanguageLocation> locations = qvariant_cast<QList<LanguageLocation>>(resultSpy.first().at(1));
	QCOMPARE(locations.size(), 1);
	QCOMPARE(locations.first().document, request.document);
	QCOMPARE(locations.first().range.start.line, 2);
	QCOMPARE(locations.first().range.start.character, 3);
	QCOMPARE(locations.first().range.end.character, 4);

	QFile log(logPath);
	QVERIFY(log.open(QIODevice::ReadOnly));
	const QJsonObject message = QJsonDocument::fromJson(log.readLine().trimmed()).object();
	QCOMPARE(message.value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/definition"));
	const QJsonObject params = message.value(QStringLiteral("params")).toObject();
	QCOMPARE(params.value(QStringLiteral("textDocument")).toObject().value(QStringLiteral("uri")).toString(),
	         request.document.toString(QUrl::FullyEncoded));
	QCOMPARE(params.value(QStringLiteral("position")).toObject().value(QStringLiteral("line")).toInt(), 2);
	QCOMPARE(params.value(QStringLiteral("position")).toObject().value(QStringLiteral("character")).toInt(), 3);
	stopAndWait(service);
}

void LanguageServiceSessionTest::definitionResults_data()
{
	QTest::addColumn<QString>("mode");
	QTest::addColumn<int>("locationCount");
	QTest::addColumn<bool>("fails");
	QTest::newRow("null") << QStringLiteral("null") << 0 << false;
	QTest::newRow("Location") << QStringLiteral("location") << 1 << false;
	QTest::newRow("Location array") << QStringLiteral("array") << 2 << false;
	QTest::newRow("LocationLink array") << QStringLiteral("link") << 1 << false;
	QTest::newRow("delayed Location") << QStringLiteral("delayed") << 1 << false;
	QTest::newRow("non-file URI") << QStringLiteral("nonfile") << 1 << false;
	QTest::newRow("malformed") << QStringLiteral("malformed") << 0 << true;
	QTest::newRow("provider error") << QStringLiteral("error") << 0 << true;
}

void LanguageServiceSessionTest::definitionResults()
{
	QFETCH(QString, mode);
	QFETCH(int, locationCount);
	QFETCH(bool, fails);
	LanguageServiceConfiguration config = configuration(QStringLiteral("digestif"));
	config.arguments << QStringLiteral("--definition-mode") << mode;
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);
	QSignalSpy resultSpy(&service, SIGNAL(definitionFinished(quint64, const QList<Tw::LanguageServices::LanguageLocation> &)));
	QSignalSpy failureSpy(&service, SIGNAL(definitionFailed(quint64, const QString &)));
	LanguageDefinitionRequest request;
	request.token = 9;
	request.document = QUrl::fromLocalFile(QStringLiteral("/tmp/source.mkxl"));
	QVERIFY(service.requestDefinition(request));
	if (fails) {
		QTRY_COMPARE(failureSpy.count(), 1);
		QCOMPARE(resultSpy.count(), 0);
	}
	else {
		QTRY_COMPARE(resultSpy.count(), 1);
		QCOMPARE(failureSpy.count(), 0);
		const QList<LanguageLocation> locations = qvariant_cast<QList<LanguageLocation>>(resultSpy.first().at(1));
		QCOMPARE(locations.size(), locationCount);
		if (mode == QStringLiteral("nonfile"))
			QVERIFY(!locations.first().document.isLocalFile());
	}
	stopAndWait(service);
}

void LanguageServiceSessionTest::unknownCapabilitiesIgnored()
{
	LspLanguageService service(configuration(QStringLiteral("unknown")), nullptr, 1000, 100, 100);
	waitForReady(service);
	QVERIFY(service.capabilities().completion);
	stopAndWait(service);
}

void LanguageServiceSessionTest::positionEncoding_data()
{
	QTest::addColumn<QString>("profile");
	QTest::addColumn<bool>("ready");
	QTest::newRow("explicit UTF-16") << QStringLiteral("digestif") << true;
	QTest::newRow("default UTF-16") << QStringLiteral("omitted") << true;
	QTest::newRow("unsupported UTF-8") << QStringLiteral("utf8") << false;
	QTest::newRow("unsupported UTF-32") << QStringLiteral("utf32") << false;
}

void LanguageServiceSessionTest::positionEncoding()
{
	QFETCH(QString, profile);
	QFETCH(bool, ready);
	LspLanguageService service(configuration(profile), nullptr, 1000, 50, 50);
	QSignalSpy failureSpy(&service, SIGNAL(failed(const QString &)));
	QVERIFY(service.start());
	if (ready) {
		QTRY_COMPARE(service.state(), LanguageService::Ready);
		stopAndWait(service);
	}
	else {
		QTRY_COMPARE(service.state(), LanguageService::Failed);
		QCOMPARE(failureSpy.count(), 1);
		QVERIFY(service.failureReason().contains(QStringLiteral("unsupported")));
		QVERIFY(capabilitiesAreEmpty(service.capabilities()));
	}
}

void LanguageServiceSessionTest::initializeFailure_data()
{
	QTest::addColumn<QString>("profile");
	QTest::newRow("JSON-RPC error") << QStringLiteral("initialize-error");
	QTest::newRow("malformed result") << QStringLiteral("malformed-initialize");
	QTest::newRow("invalid sync shape") << QStringLiteral("invalid-sync");
}

void LanguageServiceSessionTest::initializeFailure()
{
	QFETCH(QString, profile);
	LspLanguageService service(configuration(profile), nullptr, 1000, 50, 50);
	QSignalSpy failureSpy(&service, SIGNAL(failed(const QString &)));
	QVERIFY(service.start());
	QTRY_COMPARE(service.state(), LanguageService::Failed);
	QCOMPARE(failureSpy.count(), 1);
	QVERIFY(!service.failureReason().isEmpty());
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::processExitWhileInitializing()
{
	LspLanguageService service(configuration(QStringLiteral("initialize-exit")), nullptr, 1000, 50, 50);
	QSignalSpy failureSpy(&service, SIGNAL(failed(const QString &)));
	QVERIFY(service.start());
	QTRY_COMPARE(service.state(), LanguageService::Failed);
	QCOMPARE(failureSpy.count(), 1);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::unexpectedExitAfterReady()
{
	LspLanguageService service(configuration(QStringLiteral("exit-after-initialized")), nullptr, 1000, 50, 50);
	QSignalSpy failureSpy(&service, SIGNAL(failed(const QString &)));
	QVERIFY(service.start());
	QTRY_COMPARE(service.state(), LanguageService::Failed);
	QCOMPARE(failureSpy.count(), 1);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::unsupportedServerRequestIsRejected()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString confirmationPath = QDir(directory.path()).filePath(QStringLiteral("server-request.txt"));
	LanguageServiceConfiguration config = configuration(QStringLiteral("unsupported-request"));
	config.arguments << QStringLiteral("--confirmation-log") << confirmationPath;
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);
	QTRY_VERIFY(QFileInfo(confirmationPath).size() > 0);
	stopAndWait(service);

	QFile confirmation(confirmationPath);
	QVERIFY(confirmation.open(QIODevice::ReadOnly));
	QCOMPARE(confirmation.readAll(), QByteArrayLiteral("unsupported request rejected\n"));
}

void LanguageServiceSessionTest::gracefulShutdown()
{
	LspLanguageService service(configuration(QStringLiteral("digestif")), nullptr, 1000, 100, 100);
	waitForReady(service);
	service.stop();
	QTRY_COMPARE(service.state(), LanguageService::Stopped);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::shutdownTimeoutIsBounded()
{
	LspLanguageService service(configuration(QStringLiteral("shutdown-timeout")), nullptr, 1000, 30, 30);
	waitForReady(service);
	QElapsedTimer elapsed;
	elapsed.start();
	service.stop();
	QTRY_COMPARE(service.state(), LanguageService::Stopped);
	QVERIFY(elapsed.elapsed() < 1000);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::stopDuringInitialization()
{
	LspLanguageService service(configuration(QStringLiteral("delayed-initialize")), nullptr, 1000, 30, 30);
	QVERIFY(service.start());
	QTRY_COMPARE(service.state(), LanguageService::Initializing);
	service.stop();
	QTRY_COMPARE(service.state(), LanguageService::Stopped);
	QTest::qWait(350);
	QCOMPARE(service.state(), LanguageService::Stopped);
	QVERIFY(capabilitiesAreEmpty(service.capabilities()));
}

void LanguageServiceSessionTest::sessionGenerationIsDistinct()
{
	LspLanguageService first(configuration(QStringLiteral("omitted")), nullptr, 1000, 50, 50);
	LspLanguageService second(configuration(QStringLiteral("omitted")), nullptr, 1000, 50, 50);
	QVERIFY(first.start());
	QVERIFY(second.start());
	QVERIFY(first.generation() != 0);
	QVERIFY(second.generation() != 0);
	QVERIFY(first.generation() != second.generation());
	first.stop();
	second.stop();
	QTRY_COMPARE(first.state(), LanguageService::Stopped);
	QTRY_COMPARE(second.state(), LanguageService::Stopped);
}

void LanguageServiceSessionTest::managerOwnership()
{
	LanguageServiceManager manager;
	QCOMPARE(manager.state(), LanguageService::NotConfigured);
	QVERIFY(capabilitiesAreEmpty(manager.capabilities()));
	LspLanguageService * service = new LspLanguageService(configuration(QStringLiteral("digestif")), nullptr, 1000, 100, 100);
	QVERIFY(manager.setService(service));
	QCOMPARE(service->parent(), &manager);
	QVERIFY(manager.start());
	QTRY_COMPARE(manager.state(), LanguageService::Ready);
	QVERIFY(manager.capabilities().completion);
	manager.stop();
	QTRY_COMPARE(manager.state(), LanguageService::Stopped);
	QVERIFY(capabilitiesAreEmpty(manager.capabilities()));
}

void LanguageServiceSessionTest::managerPendingReplacementOwnership()
{
	{
		auto * manager = new LanguageServiceManager;
		auto * active = new ControlledStopService;
		auto * pending = new ControlledStopService;
		QPointer<ControlledStopService> activeGuard(active);
		QPointer<ControlledStopService> pendingGuard(pending);
		int activeDestroyed = 0;
		int pendingDestroyed = 0;
		connect(active, &QObject::destroyed, [&activeDestroyed]() { ++activeDestroyed; });
		connect(pending, &QObject::destroyed, [&pendingDestroyed]() { ++pendingDestroyed; });
		QVERIFY(manager->setService(active));
		QVERIFY(manager->start());
		QVERIFY(manager->replaceService(pending));
		QCOMPARE(pending->parent(), manager);

		delete manager;
		QVERIFY(activeGuard.isNull());
		QVERIFY(pendingGuard.isNull());
		QCOMPARE(activeDestroyed, 1);
		QCOMPARE(pendingDestroyed, 1);
	}

	LanguageServiceManager manager;
	auto * active = new ControlledStopService;
	QVERIFY(manager.setService(active));
	QVERIFY(manager.start());
	const quint64 activeGeneration = active->generation();
	auto * firstPending = new ControlledStopService;
	QPointer<ControlledStopService> firstPendingGuard(firstPending);
	int firstPendingDestroyed = 0;
	connect(firstPending, &QObject::destroyed, [&firstPendingDestroyed]() { ++firstPendingDestroyed; });
	QVERIFY(manager.replaceService(firstPending));
	QVERIFY(manager.replaceService(firstPending));
	QCOMPARE(firstPendingDestroyed, 0);

	auto * latestPending = new ControlledStopService;
	QPointer<ControlledStopService> latestPendingGuard(latestPending);
	int latestPendingDestroyed = 0;
	connect(latestPending, &QObject::destroyed, [&latestPendingDestroyed]() { ++latestPendingDestroyed; });
	QVERIFY(manager.replaceService(latestPending));
	QVERIFY(firstPendingGuard.isNull());
	QCOMPARE(firstPendingDestroyed, 1);
	QCOMPARE(latestPending->parent(), &manager);

	active->finishStop();
	QCOMPARE(manager.service(), static_cast<LanguageService *>(latestPending));
	QCOMPARE(latestPending->state(), LanguageService::Ready);
	QVERIFY(latestPending->generation() != activeGeneration);

	auto * stoppedPending = new ControlledStopService;
	QPointer<ControlledStopService> stoppedPendingGuard(stoppedPending);
	QVERIFY(manager.replaceService(stoppedPending));
	QCOMPARE(stoppedPending->parent(), &manager);
	manager.stop();
	QVERIFY(stoppedPendingGuard.isNull());
	QCOMPARE(manager.service(), static_cast<LanguageService *>(latestPending));
}

void LanguageServiceSessionTest::documentLifecycleMapping()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString logPath = QDir(directory.path()).filePath(QStringLiteral("documents.jsonl"));
	LanguageServiceConfiguration config = configuration(QStringLiteral("digestif"));
	config.arguments << QStringLiteral("--document-log") << logPath;
	LspLanguageService service(config, nullptr, 1000, 100, 100);
	waitForReady(service);

	LanguageDocumentOpen open;
	open.url = QUrl::fromLocalFile(QDir(directory.path()).filePath(QStringLiteral("space name.mkxl")));
	open.languageId = QStringLiteral("context");
	open.version = 1;
	open.text = QString::fromUtf8("a😀\nb");
	QVERIFY(service.openDocument(open));

	LanguageDocumentChange full;
	full.text = QStringLiteral("complete\ntext");
	QVERIFY(service.changeDocument(open.url, 2, full));

	LanguageDocumentChange incremental;
	incremental.hasRange = true;
	incremental.range.start.line = 0;
	incremental.range.start.character = 1;
	incremental.range.end.line = 0;
	incremental.range.end.character = 3;
	incremental.text = QStringLiteral("X");
	QVERIFY(service.changeDocument(open.url, 3, incremental));
	QVERIFY(service.closeDocument(open.url));

	const auto loggedLineCount = [&logPath]() {
		QFile current(logPath);
		if (!current.open(QIODevice::ReadOnly))
			return 0;
		int count = 0;
		while (!current.atEnd()) {
			if (!current.readLine().trimmed().isEmpty())
				++count;
		}
		return count;
	};
	QTRY_COMPARE(loggedLineCount(), 4);
	QFile log(logPath);
	QVERIFY(log.open(QIODevice::ReadOnly));
	QList<QJsonObject> messages;
	while (!log.atEnd()) {
		const QJsonDocument document = QJsonDocument::fromJson(log.readLine().trimmed());
		if (document.isObject())
			messages.append(document.object());
	}
	QCOMPARE(messages.size(), 4);
	QCOMPARE(messages.at(0).value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/didOpen"));
	const QJsonObject openItem = messages.at(0).value(QStringLiteral("params")).toObject()
	                             .value(QStringLiteral("textDocument")).toObject();
	QCOMPARE(openItem.value(QStringLiteral("uri")).toString(), open.url.toString(QUrl::FullyEncoded));
	QCOMPARE(openItem.value(QStringLiteral("languageId")).toString(), QStringLiteral("context"));
	QCOMPARE(openItem.value(QStringLiteral("version")).toInt(), 1);
	QCOMPARE(openItem.value(QStringLiteral("text")).toString(), open.text);
	const QJsonObject fullChange = messages.at(1).value(QStringLiteral("params")).toObject()
	                               .value(QStringLiteral("contentChanges")).toArray().first().toObject();
	QVERIFY(!fullChange.contains(QStringLiteral("range")));
	QCOMPARE(fullChange.value(QStringLiteral("text")).toString(), full.text);
	const QJsonObject rangedChange = messages.at(2).value(QStringLiteral("params")).toObject()
	                                 .value(QStringLiteral("contentChanges")).toArray().first().toObject();
	const QJsonObject range = rangedChange.value(QStringLiteral("range")).toObject();
	QCOMPARE(range.value(QStringLiteral("start")).toObject().value(QStringLiteral("character")).toInt(), 1);
	QCOMPARE(range.value(QStringLiteral("end")).toObject().value(QStringLiteral("character")).toInt(), 3);
	QCOMPARE(rangedChange.value(QStringLiteral("text")).toString(), QStringLiteral("X"));
	QCOMPARE(messages.at(3).value(QStringLiteral("method")).toString(), QStringLiteral("textDocument/didClose"));
	stopAndWait(service);
}

void LanguageServiceSessionTest::optionalConfiguredServer()
{
	const QByteArray executable = qgetenv("TEXWORKS_TEST_LANGUAGE_SERVER");
	if (executable.isEmpty())
		QSKIP("TEXWORKS_TEST_LANGUAGE_SERVER is not set");
	LanguageServiceConfiguration realConfiguration;
	realConfiguration.executable = QString::fromLocal8Bit(executable);
	LspLanguageService service(realConfiguration, nullptr, 10000, 1000, 1000);
	waitForReady(service);
	const LanguageServiceCapabilities capabilities = service.capabilities();
	QCOMPARE(capabilities.textSync, TextSyncKind::Incremental);
	QVERIFY(capabilities.openClose);
	QVERIFY(capabilities.completion);
	QVERIFY(capabilities.signatureHelp);
	QVERIFY(capabilities.hover);
	QVERIFY(capabilities.definition);
	QVERIFY(capabilities.references);
	QVERIFY(capabilities.documentSymbols);
	QVERIFY(capabilities.workspaceSymbols);
	QVERIFY(!capabilities.diagnostics);
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString rootPath = QDir(directory.path()).filePath(QStringLiteral("root.mkxl"));
	const QString childPath = QDir(directory.path()).filePath(QStringLiteral("child.mkxl"));
	const QString rootText = QStringLiteral("\\component[child]\n\\in[child:section]\n\\startsec\n");
	const QString childText = QStringLiteral("\\startsection[title=Child,reference=child:section]\n\\stopsection\n");
	for (const auto & entry : {qMakePair(rootPath, rootText), qMakePair(childPath, childText)}) {
		QFile file(entry.first);
		QVERIFY(file.open(QIODevice::WriteOnly));
		QCOMPARE(file.write(entry.second.toUtf8()), static_cast<qint64>(entry.second.toUtf8().size()));
	}
	LanguageDocumentOpen open;
	open.url = QUrl::fromLocalFile(rootPath);
	open.languageId = QStringLiteral("context");
	open.version = 1;
	open.text = rootText;
	QVERIFY(service.openDocument(open));
	LanguageDocumentChange change;
	change.hasRange = true;
	change.range.start.line = 0;
	change.range.start.character = 17;
	change.range.end = change.range.start;
	change.text = QString::fromUtf8("😀");
	QVERIFY(service.changeDocument(open.url, 2, change));
	LanguageDocumentOpen child;
	child.url = QUrl::fromLocalFile(childPath);
	child.languageId = QStringLiteral("context");
	child.version = 1;
	child.text = childText;
	QVERIFY(service.openDocument(child));
	QSignalSpy definitionSpy(&service, SIGNAL(definitionFinished(quint64, const QList<Tw::LanguageServices::LanguageLocation> &)));
	LanguageDefinitionRequest definition;
	definition.token = 77;
	definition.document = open.url;
	definition.synchronizedVersion = 2;
	definition.position.line = 1;
	definition.position.character = 5;
	QVERIFY(service.requestDefinition(definition));
	QTRY_COMPARE_WITH_TIMEOUT(definitionSpy.count(), 1, 5000);
	const QList<LanguageLocation> locations = qvariant_cast<QList<LanguageLocation>>(definitionSpy.first().at(1));
	QCOMPARE(locations.size(), 1);
	QCOMPARE(QFileInfo(locations.first().document.toLocalFile()).canonicalFilePath(), QFileInfo(childPath).canonicalFilePath());
	QVERIFY(locations.first().range.end.line > locations.first().range.start.line
	        || locations.first().range.end.character >= locations.first().range.start.character);
	QSignalSpy completionSpy(&service, SIGNAL(completionFinished(quint64, const QList<Tw::LanguageServices::CompletionItem> &)));
	LanguageCompletionRequest completion;
	completion.token = 78;
	completion.document = open.url;
	completion.synchronizedVersion = 2;
	completion.position.line = 2;
	completion.position.character = 9;
	QVERIFY(service.requestCompletion(completion));
	QTRY_COMPARE_WITH_TIMEOUT(completionSpy.count(), 1, 5000);
	const QList<CompletionItem> completionItems = qvariant_cast<QList<CompletionItem>>(completionSpy.first().at(1));
	QVERIFY(!completionItems.isEmpty());
	bool foundStartSection = false;
	for (const CompletionItem & item : completionItems) {
		if (item.label.contains(QStringLiteral("startsection"), Qt::CaseInsensitive)) {
			foundStartSection = true;
			break;
		}
	}
	QVERIFY(foundStartSection);
	QVERIFY(service.closeDocument(child.url));
	QVERIFY(service.closeDocument(open.url));
	stopAndWait(service);
}

QTEST_GUILESS_MAIN(LanguageServiceSessionTest)
