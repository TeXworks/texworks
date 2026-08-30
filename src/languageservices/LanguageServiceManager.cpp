/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/LanguageServiceManager.h"

namespace Tw {
namespace LanguageServices {

LanguageServiceManager::LanguageServiceManager(QObject * parent)
	: QObject(parent)
{
}

LanguageServiceManager::~LanguageServiceManager()
{
	stop();
}

bool LanguageServiceManager::setService(LanguageService * service)
{
	if (service == nullptr || m_service != nullptr || m_replacingService || service->parent() != nullptr)
		return false;
	installService(service);
	return true;
}

bool LanguageServiceManager::replaceService(LanguageService * service, bool startService)
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
	m_startPendingService = startService && service;

	if (!m_service) {
		finishServiceReplacement();
		return true;
	}
	if (m_replacingService)
		return true;

	m_replacingService = true;
	LanguageService * previousService = m_service;
	previousService->stop();
	if (m_replacingService && previousService->state() == LanguageService::Stopped)
		finishServiceReplacement();
	return true;
}

void LanguageServiceManager::installService(LanguageService * service)
{
	m_service = service;
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
}

void LanguageServiceManager::finishServiceReplacement()
{
	LanguageService * previousService = m_service;
	m_service = nullptr;
	m_replacingService = false;
	if (previousService) {
		disconnect(previousService, nullptr, this, nullptr);
		previousService->deleteLater();
		emit serviceChanged(nullptr);
	}

	LanguageService * service = m_pendingService;
	const bool startService = m_startPendingService;
	m_pendingService = nullptr;
	m_startPendingService = false;
	if (!service)
		return;
	installService(service);
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
		m_startPendingService = false;
	}
	m_replacingService = false;
	if (m_service)
		m_service->stop();
}

} // namespace LanguageServices
} // namespace Tw
