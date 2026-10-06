/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/LanguageServiceDocumentBinding.h"

#include "document/TeXDocument.h"

#include <QTextBlock>

#include <atomic>

namespace Tw {
namespace LanguageServices {

namespace {
std::atomic<quint64> nextCompletionToken{1};
std::atomic<quint64> nextDefinitionToken{1};
}

LanguageServiceDocumentBinding::LanguageServiceDocumentBinding(Document::TeXDocument * document,
	                                                             LanguageService * service, QObject * parent)
	: QObject(parent), m_document(document), m_service(service)
{
	Q_ASSERT(document);
	Q_ASSERT(service);
	// QTextDocument only emits granular contentsChange notifications once it
	// has a layout. Editors normally provide one; bindings also make the
	// requirement explicit for non-widget document users and tests.
	document->documentLayout();
	connect(document, &QTextDocument::contentsChange, this, &LanguageServiceDocumentBinding::handleContentsChange);
	connect(service, &LanguageService::stateChanged, this, &LanguageServiceDocumentBinding::handleServiceState);
	connect(service, &LanguageService::generationChanged, this, [this](quint64) { deactivate(false); });
	connect(service, &LanguageService::capabilitiesChanged, this,
	        [this](const LanguageServiceCapabilities &) {
		        updateCompletionAvailability();
		        updateDefinitionAvailability();
	        });
	connect(service, &LanguageService::completionFinished, this,
	        &LanguageServiceDocumentBinding::handleCompletionResult);
	connect(service, &LanguageService::completionFailed, this,
	        [this](quint64 token, const QString &) {
		        if (token == m_completionToken) {
			        cancelCompletionRequest();
			        emit completionReady({});
		        }
	        });
	connect(service, &LanguageService::definitionFinished, this,
	        &LanguageServiceDocumentBinding::handleDefinitionResult);
	connect(service, &LanguageService::definitionFailed, this,
	        [this](quint64 token, const QString &) {
		        if (token == m_definitionToken)
			        cancelDefinitionRequest();
	        });
}

void LanguageServiceDocumentBinding::updateIdentity(const QUrl & url, const QString & languageId)
{
	if (m_closed || (m_url == url && m_languageId == languageId))
		return;
	deactivate(true);
	m_url = url;
	m_languageId = languageId;
	++m_identityGeneration;
	m_version = 0;
	m_shadow.clear();
	activate();
	updateCompletionAvailability();
	updateDefinitionAvailability();
}

void LanguageServiceDocumentBinding::prepareServiceStop()
{
	deactivate(true);
}

void LanguageServiceDocumentBinding::close()
{
	if (m_closed)
		return;
	m_closed = true;
	m_completionEditDepth = 0;
	cancelCompletionRequest();
	cancelDefinitionRequest();
	deactivate(true);
	++m_identityGeneration;
	disconnect(this);
	if (m_document)
		disconnect(m_document, nullptr, this, nullptr);
	if (m_service)
		disconnect(m_service, nullptr, this, nullptr);
}

bool LanguageServiceDocumentBinding::canRequestCompletion() const
{
	if (m_closed || !m_document || !m_service || !m_service->isReady()
	    || !m_service->capabilities().completion || !m_synchronized
	    || m_serviceGeneration != m_service->generation() || m_url.isEmpty()
	    || m_languageId.isEmpty())
		return false;
	return !m_service->capabilities().openClose || m_openOnServer;
}

bool LanguageServiceDocumentBinding::requestCompletion(const LanguagePosition & position)
{
	cancelCompletionRequest();
	if (!canRequestCompletion() || !isPositionValid(position))
		return false;
	LanguageCompletionRequest request;
	request.token = nextCompletionToken.fetch_add(1);
	request.document = m_url;
	request.synchronizedVersion = m_version;
	request.position = position;
	m_completionToken = request.token;
	m_completionServiceGeneration = m_serviceGeneration;
	m_completionIdentityGeneration = m_identityGeneration;
	m_completionVersion = m_version;
	if (!m_service->requestCompletion(request)) {
		cancelCompletionRequest();
		return false;
	}
	return true;
}

void LanguageServiceDocumentBinding::cancelCompletionRequest()
{
	if (m_completionEditDepth > 0)
		return;
	m_completionToken = 0;
	m_completionServiceGeneration = 0;
	m_completionIdentityGeneration = 0;
	m_completionVersion = 0;
}

void LanguageServiceDocumentBinding::beginCompletionEdit()
{
	++m_completionEditDepth;
}

void LanguageServiceDocumentBinding::endCompletionEdit()
{
	if (m_completionEditDepth > 0)
		--m_completionEditDepth;
}

bool LanguageServiceDocumentBinding::canRequestDefinition() const
{
	if (m_closed || !m_document || !m_service || !m_service->isReady()
	    || !m_service->capabilities().definition || !m_synchronized
	    || m_serviceGeneration != m_service->generation() || m_url.isEmpty()
	    || m_languageId.isEmpty())
		return false;
	return !m_service->capabilities().openClose || m_openOnServer;
}

bool LanguageServiceDocumentBinding::requestDefinition(const LanguagePosition & position)
{
	cancelDefinitionRequest();
	if (!canRequestDefinition() || !isPositionValid(position))
		return false;

	LanguageDefinitionRequest request;
	request.token = nextDefinitionToken.fetch_add(1);
	request.document = m_url;
	request.synchronizedVersion = m_version;
	request.position = position;
	m_definitionToken = request.token;
	m_definitionServiceGeneration = m_serviceGeneration;
	m_definitionIdentityGeneration = m_identityGeneration;
	m_definitionVersion = m_version;
	if (!m_service->requestDefinition(request)) {
		cancelDefinitionRequest();
		return false;
	}
	return true;
}

void LanguageServiceDocumentBinding::cancelDefinitionRequest()
{
	m_definitionToken = 0;
	m_definitionServiceGeneration = 0;
	m_definitionIdentityGeneration = 0;
	m_definitionVersion = 0;
}

bool LanguageServiceDocumentBinding::isPositionValid(const LanguagePosition & position) const
{
	if (!m_document || position.line < 0 || position.character < 0)
		return false;
	const QTextBlock block = m_document->findBlockByNumber(position.line);
	return block.isValid() && position.character <= block.text().size();
}

LanguagePosition LanguageServiceDocumentBinding::positionInText(const QString & text, int offset)
{
	const int textSize = static_cast<int>(text.size());
	offset = qBound(0, offset, textSize);
	LanguagePosition result;
	int lineStart = 0;
	for (int index = 0; index < offset; ++index) {
		if (text.at(index) == QLatin1Char('\n')) {
			++result.line;
			lineStart = index + 1;
		}
	}
	result.character = offset - lineStart;
	return result;
}

void LanguageServiceDocumentBinding::handleContentsChange(int position, int charsRemoved, int charsAdded)
{
	if (m_completionEditDepth == 0)
		cancelCompletionRequest();
	cancelDefinitionRequest();
	if (!m_synchronized || !m_document || !m_service || !m_service->isReady()
	    || m_serviceGeneration != m_service->generation())
		return;

	const TextSyncKind syncKind = m_service->capabilities().textSync;
	if (syncKind == TextSyncKind::None)
		return;

	const QString currentText = m_document->canonicalText();
	LanguageDocumentChange change;
	if (syncKind == TextSyncKind::Full) {
		change.text = currentText;
	}
	else {
		const int shadowSize = static_cast<int>(m_shadow.size());
		const int boundedPosition = qBound(0, position, shadowSize);
		const int boundedRemoved = qBound(0, charsRemoved, shadowSize - boundedPosition);
		change.hasRange = true;
		change.range.start = positionInText(m_shadow, boundedPosition);
		change.range.end = positionInText(m_shadow, boundedPosition + boundedRemoved);
		change.text = currentText.mid(boundedPosition, charsAdded);
		QString updatedShadow = m_shadow;
		updatedShadow.replace(boundedPosition, boundedRemoved, change.text);
		if (updatedShadow != currentText) {
			change.range.start = positionInText(m_shadow, 0);
			change.range.end = positionInText(m_shadow, shadowSize);
			change.text = currentText;
		}
		m_shadow = currentText;
		Q_ASSERT(m_shadow == m_document->canonicalText());
	}

	++m_version;
	if (m_completionEditDepth > 0 && m_completionToken != 0)
		m_completionVersion = m_version;
	if (!m_service->changeDocument(m_url, m_version, change))
		deactivate(false);
}

void LanguageServiceDocumentBinding::handleServiceState(LanguageService::State state)
{
	if (state == LanguageService::Ready)
		activate();
	else
		deactivate(false);
	updateCompletionAvailability();
	updateDefinitionAvailability();
}

void LanguageServiceDocumentBinding::handleCompletionResult(quint64 token, const QList<CompletionItem> & items)
{
	if (token == 0 || token != m_completionToken || !canRequestCompletion()
	    || m_completionServiceGeneration != m_serviceGeneration
	    || m_completionIdentityGeneration != m_identityGeneration
	    || m_completionVersion != m_version)
		return;
	m_completionToken = 0;
	m_completionServiceGeneration = 0;
	m_completionIdentityGeneration = 0;
	m_completionVersion = 0;
	emit completionReady(items);
}

void LanguageServiceDocumentBinding::handleDefinitionResult(quint64 token, const QList<LanguageLocation> & locations)
{
	if (token == 0 || token != m_definitionToken || !canRequestDefinition()
	    || m_definitionServiceGeneration != m_serviceGeneration
	    || m_definitionIdentityGeneration != m_identityGeneration
	    || m_definitionVersion != m_version)
		return;
	cancelDefinitionRequest();
	emit definitionReady(locations);
}

void LanguageServiceDocumentBinding::activate()
{
	if (m_closed || m_synchronized || !m_document || !m_service || !m_service->isReady()
	    || m_url.isEmpty() || m_languageId.isEmpty())
		return;

	m_serviceGeneration = m_service->generation();
	m_version = 1;
	const QString text = m_document->canonicalText();
	if (m_service->capabilities().textSync == TextSyncKind::Incremental)
		m_shadow = text;
	else
		m_shadow.clear();
	m_synchronized = true;
	if (m_service->capabilities().openClose) {
		LanguageDocumentOpen document;
		document.url = m_url;
		document.languageId = m_languageId;
		document.version = m_version;
		document.text = text;
		m_openOnServer = m_service->openDocument(document);
		if (!m_openOnServer)
			deactivate(false);
	}
	updateDefinitionAvailability();
	updateCompletionAvailability();
}

void LanguageServiceDocumentBinding::deactivate(bool notifyClose)
{
	m_completionEditDepth = 0;
	cancelCompletionRequest();
	cancelDefinitionRequest();
	if (notifyClose && m_openOnServer && m_service && m_service->isReady()
	    && m_serviceGeneration == m_service->generation())
		m_service->closeDocument(m_url);
	m_synchronized = false;
	m_openOnServer = false;
	m_serviceGeneration = 0;
	m_shadow.clear();
	updateDefinitionAvailability();
	updateCompletionAvailability();
}

void LanguageServiceDocumentBinding::updateCompletionAvailability()
{
	const bool available = canRequestCompletion();
	if (available == m_completionAvailable)
		return;
	m_completionAvailable = available;
	emit completionAvailabilityChanged(available);
}

void LanguageServiceDocumentBinding::updateDefinitionAvailability()
{
	const bool available = canRequestDefinition();
	if (available == m_definitionAvailable)
		return;
	m_definitionAvailable = available;
	emit definitionAvailabilityChanged(available);
}

} // namespace LanguageServices
} // namespace Tw
