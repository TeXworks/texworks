/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/lsp/LspLanguageService.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QPair>

#include <initializer_list>

namespace Tw {
namespace LanguageServices {
namespace Lsp {

namespace {

QJsonObject jsonObject(const std::initializer_list<QPair<QString, QJsonValue>> & members)
{
	QJsonObject object;
	for (const QPair<QString, QJsonValue> & member : members)
		object.insert(member.first, member.second);
	return object;
}

QJsonArray jsonArray(const std::initializer_list<QJsonValue> & values)
{
	QJsonArray array;
	for (const QJsonValue & value : values)
		array.append(value);
	return array;
}

bool capabilityEnabled(const QJsonValue & value)
{
	return value.isObject() || (value.isBool() && value.toBool());
}

QString errorResponseDescription(const QJsonObject & error)
{
	const QString message = error.value(QStringLiteral("message")).toString();
	return message.isEmpty() ? LspLanguageService::tr("Language server rejected initialization")
	                         : LspLanguageService::tr("Language server rejected initialization: %1").arg(message);
}

} // namespace

LspLanguageService::LspLanguageService(const LanguageServiceConfiguration & configuration, QObject * parent,
	                                   int initializeTimeoutMs, int shutdownTimeoutMs, int terminateTimeoutMs)
	: LanguageService(parent),
	  m_configuration(configuration),
	  m_initializeTimeoutMs(qMax(0, initializeTimeoutMs)),
	  m_shutdownTimeoutMs(qMax(0, shutdownTimeoutMs)),
	  m_terminateTimeoutMs(qMax(0, terminateTimeoutMs))
{
	m_initializeTimer.setSingleShot(true);
	m_shutdownTimer.setSingleShot(true);

	connect(&m_process, &LanguageServerProcess::stateChanged, this, [this](LanguageServerProcess::State state) {
		if (state == LanguageServerProcess::Running && this->state() == Starting)
			beginInitialize();
	});
	connect(m_process.transport(), &JsonRpcTransport::responseReceived, this, &LspLanguageService::handleResponse);
	connect(m_process.transport(), &JsonRpcTransport::errorResponseReceived, this, &LspLanguageService::handleErrorResponse);
	connect(m_process.transport(), &JsonRpcTransport::requestReceived, this,
	        [this](const QJsonValue & id, const QString &, const QJsonValue &) {
			m_process.transport()->sendErrorResponse(id, -32601, QStringLiteral("Method not found"));
	        });
	connect(m_process.transport(), &JsonRpcTransport::pendingRequestFailed, this, &LspLanguageService::handlePendingFailure);
	connect(&m_process, &LanguageServerProcess::processFinished, this, &LspLanguageService::handleProcessFinished);
	connect(&m_process, &LanguageServerProcess::processError, this,
	        [this](QProcess::ProcessError, const QString & description) {
			if (state() != Stopping && state() != Failed && state() != Stopped)
				initializationFailed(description);
	        });
	connect(&m_process, &LanguageServerProcess::writeFailed, this, [this](const QString & description) {
		if (state() != Stopping && state() != Failed && state() != Stopped)
			initializationFailed(description);
	});
	connect(&m_initializeTimer, &QTimer::timeout, this, [this]() {
		initializationFailed(tr("Language server initialization timed out"));
	});
	connect(&m_shutdownTimer, &QTimer::timeout, this, &LspLanguageService::sendExitAndStop);
}

LspLanguageService::~LspLanguageService()
{
	m_initializeTimer.stop();
	m_shutdownTimer.stop();
}

bool LspLanguageService::start()
{
	if (!beginStart())
		return false;
	if (m_configuration.executable.isEmpty()) {
		initializationFailed(tr("Language server is not configured with an executable"));
		return false;
	}
	m_teardownStarted = false;
	m_initializeRequestId = -1;
	m_shutdownRequestId = -1;
	if (!m_process.start(m_configuration.executable, m_configuration.arguments,
	                     m_configuration.environment, m_configuration.workingDirectory)) {
		initializationFailed(tr("Could not start the language server process"));
		return false;
	}
	return true;
}

void LspLanguageService::beginInitialize()
{
	setState(Initializing);
	const QJsonObject clientCapabilities = jsonObject({
		{QStringLiteral("general"), jsonObject({
			{QStringLiteral("positionEncodings"), jsonArray({QStringLiteral("utf-16")})}
		})}
	});
	const QJsonObject parameters = jsonObject({
		{QStringLiteral("processId"), static_cast<double>(QCoreApplication::applicationPid())},
		{QStringLiteral("rootUri"), QJsonValue(QJsonValue::Null)},
		{QStringLiteral("capabilities"), clientCapabilities}
	});
	m_initializeRequestId = m_process.transport()->sendRequest(QStringLiteral("initialize"), parameters);
	if (m_initializeRequestId < 0) {
		initializationFailed(tr("Could not send the language server initialize request"));
		return;
	}
	m_initializeTimer.start(m_initializeTimeoutMs);
}

void LspLanguageService::handleResponse(qint64 id, const QJsonValue & result)
{
	if (id == m_initializeRequestId) {
		m_initializeRequestId = -1;
		m_initializeTimer.stop();
		if (state() != Initializing)
			return;
		if (!result.isObject()) {
			initializationFailed(tr("Language server returned an invalid initialize result"));
			return;
		}
		const QJsonValue capabilitiesValue = result.toObject().value(QStringLiteral("capabilities"));
		if (!capabilitiesValue.isObject()) {
			initializationFailed(tr("Language server initialize result has no valid capabilities object"));
			return;
		}
		const QJsonObject capabilities = capabilitiesValue.toObject();
		const QJsonValue positionEncoding = capabilities.value(QStringLiteral("positionEncoding"));
		if (!positionEncoding.isUndefined() && (!positionEncoding.isString() || positionEncoding.toString() != QStringLiteral("utf-16"))) {
			initializationFailed(positionEncoding.isString()
			                     ? tr("Language server selected unsupported position encoding: %1").arg(positionEncoding.toString())
			                     : tr("Language server returned an invalid position encoding"));
			return;
		}
		LanguageServiceCapabilities mapped;
		QString mappingError;
		if (!mapCapabilities(capabilities, mapped, mappingError)) {
			initializationFailed(mappingError);
			return;
		}
		if (!m_process.transport()->sendNotification(QStringLiteral("initialized"), QJsonObject{})) {
			initializationFailed(tr("Could not send the language server initialized notification"));
			return;
		}
		becomeReady(mapped);
		return;
	}

	if (id == m_shutdownRequestId) {
		m_shutdownRequestId = -1;
		if (state() == Stopping)
			sendExitAndStop();
	}
}

void LspLanguageService::handleErrorResponse(qint64 id, const QJsonObject & error)
{
	if (id == m_initializeRequestId) {
		m_initializeRequestId = -1;
		m_initializeTimer.stop();
		if (state() == Initializing)
			initializationFailed(errorResponseDescription(error));
	}
	else if (id == m_shutdownRequestId) {
		m_shutdownRequestId = -1;
		if (state() == Stopping)
			sendExitAndStop();
	}
}

void LspLanguageService::handlePendingFailure(qint64 id, const QString & reason)
{
	if (id == m_initializeRequestId && state() == Initializing) {
		m_initializeRequestId = -1;
		m_initializeTimer.stop();
		initializationFailed(reason);
	}
}

void LspLanguageService::handleProcessFinished(int, QProcess::ExitStatus, bool unexpected)
{
	m_initializeTimer.stop();
	m_shutdownTimer.stop();
	if (unexpected) {
		initializationFailed(tr("Language server process exited unexpectedly"));
		return;
	}
	if (state() == Stopping)
		becomeStopped();
}

bool LspLanguageService::mapCapabilities(const QJsonObject & capabilities,
	                                     LanguageServiceCapabilities & mapped, QString & error) const
{
	const QJsonValue sync = capabilities.value(QStringLiteral("textDocumentSync"));
	int syncKind = 0;
	if (sync.isUndefined() || sync.isNull()) {
		syncKind = 0;
	}
	else if (sync.isDouble()) {
		const double numeric = sync.toDouble();
		if (numeric != static_cast<int>(numeric) || numeric < 0 || numeric > 2) {
			error = tr("Language server returned an invalid text synchronization kind");
			return false;
		}
		syncKind = static_cast<int>(numeric);
	}
	else if (sync.isObject()) {
		const QJsonObject options = sync.toObject();
		mapped.openClose = options.value(QStringLiteral("openClose")).isBool()
		                   && options.value(QStringLiteral("openClose")).toBool();
		const QJsonValue change = options.value(QStringLiteral("change"));
		if (!change.isUndefined() && !change.isNull()) {
			if (!change.isDouble() || change.toDouble() != static_cast<int>(change.toDouble())
			    || change.toDouble() < 0 || change.toDouble() > 2) {
				error = tr("Language server returned an invalid text synchronization change kind");
				return false;
			}
			syncKind = change.toInt();
		}
	}
	else {
		error = tr("Language server returned an invalid text synchronization capability");
		return false;
	}

	mapped.textSync = syncKind == 1 ? TextSyncKind::Full
	                : syncKind == 2 ? TextSyncKind::Incremental
	                                : TextSyncKind::None;
	mapped.completion = capabilityEnabled(capabilities.value(QStringLiteral("completionProvider")));
	mapped.signatureHelp = capabilityEnabled(capabilities.value(QStringLiteral("signatureHelpProvider")));
	mapped.hover = capabilityEnabled(capabilities.value(QStringLiteral("hoverProvider")));
	mapped.definition = capabilityEnabled(capabilities.value(QStringLiteral("definitionProvider")));
	mapped.references = capabilityEnabled(capabilities.value(QStringLiteral("referencesProvider")));
	mapped.documentSymbols = capabilityEnabled(capabilities.value(QStringLiteral("documentSymbolProvider")));
	mapped.workspaceSymbols = capabilityEnabled(capabilities.value(QStringLiteral("workspaceSymbolProvider")));
	// Neither push diagnostics nor pull diagnostics are consumed in this cycle.
	mapped.diagnostics = false;
	return true;
}

void LspLanguageService::initializationFailed(const QString & reason)
{
	if (state() == Failed || state() == Stopped)
		return;
	m_initializeTimer.stop();
	m_shutdownTimer.stop();
	becomeFailed(reason);
	beginProcessStop();
}

void LspLanguageService::stop()
{
	if (state() == Stopped || state() == NotConfigured || state() == Stopping)
		return;
	if (state() == Failed) {
		beginProcessStop();
		return;
	}
	const bool wasReady = state() == Ready;
	setState(Stopping);
	m_initializeTimer.stop();
	if (!wasReady) {
		beginProcessStop();
		return;
	}
	m_shutdownRequestId = m_process.transport()->sendRequest(QStringLiteral("shutdown"));
	if (m_shutdownRequestId < 0) {
		sendExitAndStop();
		return;
	}
	m_shutdownTimer.start(m_shutdownTimeoutMs);
}

void LspLanguageService::sendExitAndStop()
{
	if (m_teardownStarted)
		return;
	m_shutdownTimer.stop();
	m_process.transport()->sendNotification(QStringLiteral("exit"));
	beginProcessStop();
}

void LspLanguageService::beginProcessStop()
{
	if (m_teardownStarted)
		return;
	m_teardownStarted = true;
	m_process.stop(m_shutdownTimeoutMs, m_terminateTimeoutMs);
	if (m_process.state() == LanguageServerProcess::NotRunning && state() == Stopping)
		becomeStopped();
}

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw
