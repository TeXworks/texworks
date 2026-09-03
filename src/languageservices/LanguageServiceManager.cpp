/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/LanguageServiceManager.h"

#include "document/TeXDocument.h"
#include "languageservices/LanguageServiceDocumentBinding.h"

#include <QFileInfo>
#include <QUrl>

namespace Tw {
namespace LanguageServices {

LanguageServiceManager::LanguageServiceManager(QObject * parent)
	: QObject(parent), m_deferredRefreshTimer(this)
{
	m_deferredRefreshTimer.setSingleShot(true);
	connect(&m_deferredRefreshTimer, &QTimer::timeout,
	        this, &LanguageServiceManager::refreshPendingDocuments);
}

LanguageServiceManager::~LanguageServiceManager()
{
	stop();
}

bool LanguageServiceManager::setService(LanguageService * service, const QStringList & supportedLanguages)
{
	if (service == nullptr || m_service != nullptr || m_replacingService || service->parent() != nullptr)
		return false;
	installService(service, supportedLanguages);
	return true;
}

bool LanguageServiceManager::replaceService(LanguageService * service, const QStringList & supportedLanguages,
                                             bool startService)
{
	if (service && service == m_pendingService)
		return true;
	if ((service && service->parent()) || service == m_service)
		return false;
	if (m_pendingService)
		delete m_pendingService;
	m_pendingService = service;
	if (m_pendingService)
		m_pendingService->setParent(this);
	m_pendingSupportedLanguages = supportedLanguages;
	m_startPendingService = startService && service;

	if (!m_service) {
		finishServiceReplacement();
		return true;
	}
	if (m_replacingService)
		return true;

	m_replacingService = true;
	for (LanguageServiceDocumentBinding * binding : m_bindings) {
		binding->prepareServiceStop();
		delete binding;
	}
	m_bindings.clear();
	LanguageService * previousService = m_service;
	previousService->stop();
	if (m_replacingService && previousService->state() == LanguageService::Stopped)
		finishServiceReplacement();
	return true;
}

void LanguageServiceManager::installService(LanguageService * service, const QStringList & supportedLanguages)
{
	m_service = service;
	m_supportedLanguages = supportedLanguages;
	m_service->setParent(this);
	connect(m_service, &LanguageService::stateChanged, this,
	        [this, service](LanguageService::State state) {
		        if (service != m_service)
			        return;
		        emit stateChanged(state);
		        if (m_replacingService && state == LanguageService::Stopped)
			        finishServiceReplacement();
	        });
	connect(m_service, &LanguageService::capabilitiesChanged, this, &LanguageServiceManager::capabilitiesChanged);
	connect(m_service, &LanguageService::failed, this, &LanguageServiceManager::failed);
	emit serviceChanged(m_service);
	for (Document::TeXDocument * document : m_documents.keys())
		createBinding(document);
}

void LanguageServiceManager::finishServiceReplacement()
{
	LanguageService * previousService = m_service;
	m_service = nullptr;
	m_supportedLanguages.clear();
	m_replacingService = false;
	if (previousService) {
		disconnect(previousService, nullptr, this, nullptr);
		previousService->deleteLater();
		emit serviceChanged(nullptr);
	}

	LanguageService * service = m_pendingService;
	const QStringList supportedLanguages = m_pendingSupportedLanguages;
	const bool startService = m_startPendingService;
	m_pendingService = nullptr;
	m_pendingSupportedLanguages.clear();
	m_startPendingService = false;
	if (!service)
		return;
	installService(service, supportedLanguages);
	if (startService)
		start();
}

LanguageService::State LanguageServiceManager::state() const
{
	return m_service ? m_service->state() : LanguageService::NotConfigured;
}

LanguageServiceCapabilities LanguageServiceManager::capabilities() const
{
	return m_service ? m_service->capabilities() : LanguageServiceCapabilities{};
}

bool LanguageServiceManager::start()
{
	return m_service && m_service->start();
}

void LanguageServiceManager::stop()
{
	if (m_pendingService) {
		delete m_pendingService;
		m_pendingService = nullptr;
		m_pendingSupportedLanguages.clear();
		m_startPendingService = false;
	}
	m_replacingService = false;
	for (LanguageServiceDocumentBinding * binding : m_bindings)
		binding->prepareServiceStop();
	if (m_service)
		m_service->stop();
}

void LanguageServiceManager::registerDocument(Document::TeXDocument * document, const QString & sourceLanguage)
{
	if (!document || m_documents.contains(document))
		return;
	m_documents.insert(document, sourceLanguage);
	connect(document, &Document::TeXDocument::modelinesChanged, this,
	        [this, document](const QStringList &, const QStringList &) {
		        scheduleDocumentRefresh(document);
	        });
	connect(document, &QObject::destroyed, this, [this, document]() { unregisterDocument(document); });
	createBinding(document);
}

void LanguageServiceManager::unregisterDocument(Document::TeXDocument * document)
{
	for (int i = static_cast<int>(m_pendingDocumentRefreshes.size()); i-- > 0;) {
		if (!m_pendingDocumentRefreshes.at(i) || m_pendingDocumentRefreshes.at(i).data() == document)
			m_pendingDocumentRefreshes.removeAt(i);
	}
	LanguageServiceDocumentBinding * binding = m_bindings.take(document);
	if (binding) {
		binding->close();
		delete binding;
	}
	m_documents.remove(document);
}

void LanguageServiceManager::updateDocumentIdentity(Document::TeXDocument * document)
{
	refreshBinding(document);
}

void LanguageServiceManager::updateDocumentLanguageHint(Document::TeXDocument * document, const QString & sourceLanguage)
{
	if (!m_documents.contains(document))
		return;
	m_documents[document] = sourceLanguage;
	refreshBinding(document);
}

LanguageServiceDocumentBinding * LanguageServiceManager::bindingForDocument(Document::TeXDocument * document) const
{
	return m_bindings.value(document, nullptr);
}

bool LanguageServiceManager::canRequestCompletion(Document::TeXDocument * document) const
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	return binding && binding->canRequestCompletion();
}

bool LanguageServiceManager::requestCompletion(Document::TeXDocument * document, const LanguagePosition & position)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	return binding && binding->requestCompletion(position);
}

void LanguageServiceManager::cancelCompletionRequest(Document::TeXDocument * document)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	if (binding)
		binding->cancelCompletionRequest();
}

void LanguageServiceManager::beginCompletionEdit(Document::TeXDocument * document)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	if (binding)
		binding->beginCompletionEdit();
}

void LanguageServiceManager::endCompletionEdit(Document::TeXDocument * document)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	if (binding)
		binding->endCompletionEdit();
}

bool LanguageServiceManager::canRequestDefinition(Document::TeXDocument * document) const
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	return binding && binding->canRequestDefinition();
}

bool LanguageServiceManager::requestDefinition(Document::TeXDocument * document, const LanguagePosition & position)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	return binding && binding->requestDefinition(position);
}

void LanguageServiceManager::cancelDefinitionRequest(Document::TeXDocument * document)
{
	LanguageServiceDocumentBinding * binding = bindingForDocument(document);
	if (binding)
		binding->cancelDefinitionRequest();
}

bool LanguageServiceManager::isSourceEligible(const Document::TeXDocument * document)
{
	return document && document->isStoredInFilesystem() && !document->absoluteFilePath().isEmpty();
}

QString LanguageServiceManager::identifyLanguageId(const Document::TeXDocument * document, const QString & sourceLanguage)
{
	if (!document)
		return {};
	const QString suffix = document->getFileInfo().suffix().toLower();
	if (suffix == QStringLiteral("mkxl") || suffix == QStringLiteral("mkiv"))
		return QStringLiteral("context");
	if (suffix != QStringLiteral("tex"))
		return {};

	if (document->hasModeLine(QStringLiteral("program"))) {
		return document->getModeLineValue(QStringLiteral("program")).trimmed().compare(
		           QStringLiteral("context"), Qt::CaseInsensitive) == 0
		           ? QStringLiteral("context")
		           : QString{};
	}
	return sourceLanguage == QStringLiteral("context") ? QStringLiteral("context") : QString{};
}

void LanguageServiceManager::createBinding(Document::TeXDocument * document)
{
	if (!document || !m_service || m_bindings.contains(document))
		return;
	LanguageServiceDocumentBinding * binding = new LanguageServiceDocumentBinding(document, m_service, this);
	m_bindings.insert(document, binding);
	connect(binding, &LanguageServiceDocumentBinding::completionAvailabilityChanged, this,
	        [this, document](bool available) { emit completionAvailabilityChanged(document, available); });
	connect(binding, &LanguageServiceDocumentBinding::completionReady, this,
	        [this, document](const QList<CompletionItem> & items) { emit completionReady(document, items); });
	connect(binding, &LanguageServiceDocumentBinding::definitionAvailabilityChanged, this,
	        [this, document](bool available) { emit definitionAvailabilityChanged(document, available); });
	connect(binding, &LanguageServiceDocumentBinding::definitionReady, this,
	        [this, document](const QList<LanguageLocation> & locations) { emit definitionReady(document, locations); });
	refreshBinding(document);
}

void LanguageServiceManager::scheduleDocumentRefresh(Document::TeXDocument * document)
{
	if (!document || !m_documents.contains(document))
		return;
	for (const QPointer<Document::TeXDocument> & pendingDocument : m_pendingDocumentRefreshes) {
		if (pendingDocument.data() == document)
			return;
	}
	m_pendingDocumentRefreshes.append(QPointer<Document::TeXDocument>(document));
	if (!m_deferredRefreshTimer.isActive())
		m_deferredRefreshTimer.start(0);
}

void LanguageServiceManager::refreshPendingDocuments()
{
	const QList<QPointer<Document::TeXDocument>> pendingDocuments = m_pendingDocumentRefreshes;
	m_pendingDocumentRefreshes.clear();
	for (const QPointer<Document::TeXDocument> & document : pendingDocuments) {
		if (document && m_documents.contains(document.data()))
			refreshBinding(document.data());
	}
}

void LanguageServiceManager::refreshBinding(Document::TeXDocument * document)
{
	LanguageServiceDocumentBinding * binding = m_bindings.value(document, nullptr);
	if (!binding || !document)
		return;
	QString languageId;
	if (isSourceEligible(document))
		languageId = identifyLanguageId(document, m_documents.value(document));
	if (!m_supportedLanguages.contains(languageId))
		languageId.clear();
	QUrl url;
	if (!languageId.isEmpty()) {
		const QFileInfo info(document->absoluteFilePath());
		const QString path = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
		url = QUrl::fromLocalFile(path);
	}
	binding->updateIdentity(url, languageId);
}

} // namespace LanguageServices
} // namespace Tw
