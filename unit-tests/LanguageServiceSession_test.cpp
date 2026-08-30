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
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using Tw::LanguageServices::LanguageService;
using Tw::LanguageServices::LanguageServiceCapabilities;
using Tw::LanguageServices::LanguageServiceConfiguration;
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
};

} // namespace

void LanguageServiceSessionTest::initTestCase()
{
	qRegisterMetaType<LanguageService::State>("Tw::LanguageServices::LanguageService::State");
	qRegisterMetaType<LanguageServiceCapabilities>("Tw::LanguageServices::LanguageServiceCapabilities");
	qRegisterMetaType<TextSyncKind>("Tw::LanguageServices::TextSyncKind");
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
	stopAndWait(service);
}

QTEST_GUILESS_MAIN(LanguageServiceSessionTest)
