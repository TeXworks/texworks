/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/LanguageService.h"

#include <atomic>

namespace Tw {
namespace LanguageServices {

namespace {
std::atomic<quint64> nextGeneration{1};
}

LanguageService::LanguageService(QObject * parent)
	: QObject(parent)
{
}

bool LanguageService::beginStart()
{
	if (m_state != Stopped)
		return false;
	m_failureReason.clear();
	clearCapabilities();
	m_generation = nextGeneration.fetch_add(1);
	emit generationChanged(m_generation);
	setState(Starting);
	return true;
}

void LanguageService::setState(State state)
{
	if (m_state == state)
		return;
	const bool wasReady = isReady();
	if (state != Ready)
		clearCapabilities();
	m_state = state;
	emit stateChanged(m_state);
	if (wasReady != isReady())
		emit readinessChanged(isReady());
}

void LanguageService::becomeReady(const LanguageServiceCapabilities & capabilities)
{
	if (m_state != Initializing)
		return;
	m_capabilities = capabilities;
	setState(Ready);
	emit capabilitiesChanged(m_capabilities);
}

void LanguageService::becomeFailed(const QString & reason)
{
	if (m_state == Failed || m_state == Stopped)
		return;
	m_failureReason = reason;
	setState(Failed);
	emit failed(m_failureReason);
}

void LanguageService::becomeStopped()
{
	m_failureReason.clear();
	setState(Stopped);
}

void LanguageService::clearCapabilities()
{
	const LanguageServiceCapabilities empty;
	if (m_capabilities == empty)
		return;
	m_capabilities = empty;
	emit capabilitiesChanged(m_capabilities);
}

} // namespace LanguageServices
} // namespace Tw
