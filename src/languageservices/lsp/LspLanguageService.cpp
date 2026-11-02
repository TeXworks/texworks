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

#include <cmath>
#include <initializer_list>
#include <limits>

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

bool parsePosition(const QJsonValue & value, LanguagePosition & position)
{
	if (!value.isObject())
		return false;
	const QJsonObject object = value.toObject();
	const QJsonValue line = object.value(QStringLiteral("line"));
	const QJsonValue character = object.value(QStringLiteral("character"));
	if (!line.isDouble() || !character.isDouble())
		return false;
	const double lineNumber = line.toDouble();
	const double characterNumber = character.toDouble();
	if (!std::isfinite(lineNumber) || !std::isfinite(characterNumber)
	    || lineNumber < 0 || characterNumber < 0
	    || lineNumber > std::numeric_limits<int>::max()
	    || characterNumber > std::numeric_limits<int>::max()
	    || lineNumber != static_cast<int>(lineNumber)
	    || characterNumber != static_cast<int>(characterNumber))
		return false;
	position.line = static_cast<int>(lineNumber);
	position.character = static_cast<int>(characterNumber);
	return true;
}

bool parseRange(const QJsonValue & value, LanguageRange & range)
{
	if (!value.isObject())
		return false;
	const QJsonObject object = value.toObject();
	if (!parsePosition(object.value(QStringLiteral("start")), range.start)
	    || !parsePosition(object.value(QStringLiteral("end")), range.end))
		return false;
	return range.end.line > range.start.line
	       || (range.end.line == range.start.line && range.end.character >= range.start.character);
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
	const auto completion = m_completionRequests.find(id);
	if (completion != m_completionRequests.end()) {
		const quint64 token = completion.value();
		m_completionRequests.erase(completion);
		QList<CompletionItem> items;
		if (mapCompletionResult(result, items))
			emit completionFinished(token, items);
		else
			emit completionFailed(token, tr("Language server returned an invalid completion result"));
		return;
	}
	const auto definition = m_definitionRequests.find(id);
	if (definition != m_definitionRequests.end()) {
		const quint64 token = definition.value();
		m_definitionRequests.erase(definition);
		QList<LanguageLocation> locations;
		if (mapDefinitionResult(result, locations))
			emit definitionFinished(token, locations);
		else
			emit definitionFailed(token, tr("Language server returned an invalid definition result"));
		return;
	}
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
	const auto completion = m_completionRequests.find(id);
	if (completion != m_completionRequests.end()) {
		const quint64 token = completion.value();
		m_completionRequests.erase(completion);
		emit completionFailed(token, error.value(QStringLiteral("message")).toString());
		return;
	}
	const auto definition = m_definitionRequests.find(id);
	if (definition != m_definitionRequests.end()) {
		const quint64 token = definition.value();
		m_definitionRequests.erase(definition);
		emit definitionFailed(token, error.value(QStringLiteral("message")).toString());
		return;
	}
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
	const auto completion = m_completionRequests.find(id);
	if (completion != m_completionRequests.end()) {
		const quint64 token = completion.value();
		m_completionRequests.erase(completion);
		emit completionFailed(token, reason);
		return;
	}
	const auto definition = m_definitionRequests.find(id);
	if (definition != m_definitionRequests.end()) {
		const quint64 token = definition.value();
		m_definitionRequests.erase(definition);
		emit definitionFailed(token, reason);
		return;
	}
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
	if (state() == Stopping) {
		becomeStopped();
		return;
	}
	if (unexpected) {
		initializationFailed(tr("Language server process exited unexpectedly"));
		return;
	}
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
	failCompletionRequests(reason);
	failDefinitionRequests(reason);
	beginProcessStop();
}

void LspLanguageService::stop()
{
	if (state() == Stopped || state() == NotConfigured || state() == Stopping)
		return;
	if (state() == Failed) {
		setState(Stopping);
		if (m_teardownStarted) {
			if (!m_process.isRunning())
				becomeStopped();
			return;
		}
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

bool LspLanguageService::openDocument(const LanguageDocumentOpen & document)
{
	if (!isReady())
		return false;
	const QJsonObject item = jsonObject({
		{QStringLiteral("uri"), document.url.toString(QUrl::FullyEncoded)},
		{QStringLiteral("languageId"), document.languageId},
		{QStringLiteral("version"), static_cast<double>(document.version)},
		{QStringLiteral("text"), document.text}
	});
	return m_process.transport()->sendNotification(QStringLiteral("textDocument/didOpen"),
	                                               jsonObject({{QStringLiteral("textDocument"), item}}));
}

bool LspLanguageService::changeDocument(const QUrl & url, quint64 version, const LanguageDocumentChange & change)
{
	if (!isReady())
		return false;
	QJsonObject contentChange = jsonObject({{QStringLiteral("text"), change.text}});
	if (change.hasRange) {
		const auto position = [](const LanguagePosition & value) {
			return jsonObject({{QStringLiteral("line"), value.line},
			                   {QStringLiteral("character"), value.character}});
		};
		contentChange.insert(QStringLiteral("range"),
		                     jsonObject({{QStringLiteral("start"), position(change.range.start)},
		                                 {QStringLiteral("end"), position(change.range.end)}}));
	}
	const QJsonObject identifier = jsonObject({
		{QStringLiteral("uri"), url.toString(QUrl::FullyEncoded)},
		{QStringLiteral("version"), static_cast<double>(version)}
	});
	return m_process.transport()->sendNotification(QStringLiteral("textDocument/didChange"),
	                                               jsonObject({{QStringLiteral("textDocument"), identifier},
	                                                           {QStringLiteral("contentChanges"), jsonArray({contentChange})}}));
}

bool LspLanguageService::closeDocument(const QUrl & url)
{
	if (!isReady())
		return false;
	return m_process.transport()->sendNotification(QStringLiteral("textDocument/didClose"),
	                                               jsonObject({{QStringLiteral("textDocument"),
	                                                            jsonObject({{QStringLiteral("uri"), url.toString(QUrl::FullyEncoded)}})}}));
}

bool LspLanguageService::requestCompletion(const LanguageCompletionRequest & request)
{
	if (!isReady() || !capabilities().completion || request.token == 0
	    || request.document.isEmpty() || request.position.line < 0 || request.position.character < 0)
		return false;
	const QJsonObject position = jsonObject({{QStringLiteral("line"), request.position.line},
	                                         {QStringLiteral("character"), request.position.character}});
	const QJsonObject identifier = jsonObject({{QStringLiteral("uri"), request.document.toString(QUrl::FullyEncoded)}});
	const QJsonObject parameters = jsonObject({{QStringLiteral("textDocument"), identifier},
	                                           {QStringLiteral("position"), position}});
	const qint64 id = m_process.transport()->sendRequest(QStringLiteral("textDocument/completion"), parameters);
	if (id < 0)
		return false;
	m_completionRequests.insert(id, request.token);
	return true;
}

bool LspLanguageService::mapCompletionResult(const QJsonValue & result, QList<CompletionItem> & items) const
{
	if (result.isNull())
		return true;
	QJsonArray values;
	if (result.isArray()) {
		values = result.toArray();
	}
	else if (result.isObject()) {
		const QJsonValue listItems = result.toObject().value(QStringLiteral("items"));
		if (!listItems.isArray())
			return false;
		values = listItems.toArray();
	}
	else {
		return false;
	}

	for (const QJsonValue & value : values) {
		if (!value.isObject())
			continue;
		const QJsonObject object = value.toObject();
		const QJsonValue label = object.value(QStringLiteral("label"));
		if (!label.isString() || label.toString().isEmpty()
		    || object.contains(QStringLiteral("additionalTextEdits"))
		    || object.contains(QStringLiteral("command")))
			continue;
		const QJsonValue format = object.value(QStringLiteral("insertTextFormat"));
		if (!format.isUndefined()
		    && (!format.isDouble() || format.toDouble() != static_cast<int>(format.toDouble())
		        || format.toInt() < 1 || format.toInt() > 2))
			continue;
		// Snippet placeholders are intentionally unsupported in this first cycle.
		if (format.toInt(1) == 2)
			continue;

		CompletionItem item;
		item.label = label.toString();
		item.detail = object.value(QStringLiteral("detail")).toString();
		const QJsonValue documentation = object.value(QStringLiteral("documentation"));
		if (documentation.isString())
			item.documentation = documentation.toString();
		else if (documentation.isObject())
			item.documentation = documentation.toObject().value(QStringLiteral("value")).toString();

		const QJsonValue textEdit = object.value(QStringLiteral("textEdit"));
		if (!textEdit.isUndefined()) {
			if (!textEdit.isObject())
				continue;
			const QJsonObject edit = textEdit.toObject();
			if (!edit.value(QStringLiteral("newText")).isString()
			    || !parseRange(edit.value(QStringLiteral("range")), item.replacementRange))
				continue;
			item.insertText = edit.value(QStringLiteral("newText")).toString();
			item.hasReplacementRange = true;
		}
		else if (object.value(QStringLiteral("insertText")).isString()) {
			item.insertText = object.value(QStringLiteral("insertText")).toString();
		}
		else {
			item.insertText = item.label;
		}
		items.append(item);
	}
	return true;
}

bool LspLanguageService::requestDefinition(const LanguageDefinitionRequest & request)
{
	if (!isReady() || !capabilities().definition || request.token == 0
	    || request.document.isEmpty() || request.position.line < 0 || request.position.character < 0)
		return false;
	const QJsonObject position = jsonObject({{QStringLiteral("line"), request.position.line},
	                                         {QStringLiteral("character"), request.position.character}});
	const QJsonObject identifier = jsonObject({{QStringLiteral("uri"), request.document.toString(QUrl::FullyEncoded)}});
	const QJsonObject parameters = jsonObject({{QStringLiteral("textDocument"), identifier},
	                                           {QStringLiteral("position"), position}});
	const qint64 id = m_process.transport()->sendRequest(QStringLiteral("textDocument/definition"), parameters);
	if (id < 0)
		return false;
	m_definitionRequests.insert(id, request.token);
	return true;
}

bool LspLanguageService::mapDefinitionResult(const QJsonValue & result, QList<LanguageLocation> & locations) const
{
	if (result.isNull())
		return true;
	QJsonArray values;
	const bool singleLocation = result.isObject();
	if (result.isArray())
		values = result.toArray();
	else if (result.isObject())
		values.append(result);
	else
		return false;

	int arrayKind = -1;
	for (const QJsonValue & value : values) {
		if (!value.isObject())
			return false;
		const QJsonObject object = value.toObject();
		const bool locationLink = object.contains(QStringLiteral("targetUri"));
		if (singleLocation && locationLink)
			return false;
		if (!singleLocation) {
			const int currentKind = locationLink ? 1 : 0;
			if (arrayKind >= 0 && currentKind != arrayKind)
				return false;
			arrayKind = currentKind;
		}
		const QJsonValue uriValue = object.value(locationLink ? QStringLiteral("targetUri") : QStringLiteral("uri"));
		const QJsonValue rangeValue = object.value(locationLink ? QStringLiteral("targetSelectionRange") : QStringLiteral("range"));
		if (!uriValue.isString())
			return false;
		LanguageLocation location;
		location.document = QUrl(uriValue.toString(), QUrl::StrictMode);
		LanguageRange targetRange;
		if (!location.document.isValid() || location.document.isEmpty() || location.document.isRelative()
		    || !parseRange(rangeValue, location.range)
		    || (locationLink && !parseRange(object.value(QStringLiteral("targetRange")), targetRange)))
			return false;
		locations.append(location);
	}
	return true;
}

void LspLanguageService::failDefinitionRequests(const QString & reason)
{
	const QHash<qint64, quint64> requests = m_definitionRequests;
	m_definitionRequests.clear();
	for (quint64 token : requests)
		emit definitionFailed(token, reason);
}

void LspLanguageService::failCompletionRequests(const QString & reason)
{
	const QHash<qint64, quint64> requests = m_completionRequests;
	m_completionRequests.clear();
	for (quint64 token : requests)
		emit completionFailed(token, reason);
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
	failCompletionRequests(tr("Language server stopped"));
	failDefinitionRequests(tr("Language server stopped"));
	m_process.stop(m_shutdownTimeoutMs, m_terminateTimeoutMs);
	if (m_process.state() == LanguageServerProcess::NotRunning && state() == Stopping)
		becomeStopped();
}

} // namespace Lsp
} // namespace LanguageServices
} // namespace Tw
