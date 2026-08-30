/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICE_H
#define LANGUAGESERVICE_H

#include "languageservices/LanguageServiceTypes.h"

#include <QObject>

namespace Tw {
namespace LanguageServices {

class LanguageService : public QObject
{
	Q_OBJECT

public:
	enum State {
		NotConfigured,
		Starting,
		Initializing,
		Ready,
		Stopping,
		Failed,
		Stopped
	};
	Q_ENUMS(State)

	explicit LanguageService(QObject * parent = nullptr);
	~LanguageService() override = default;

	State state() const { return m_state; }
	bool isReady() const { return m_state == Ready; }
	LanguageServiceCapabilities capabilities() const { return m_capabilities; }
	quint64 generation() const { return m_generation; }
	QString failureReason() const { return m_failureReason; }

	virtual bool start() = 0;
	virtual void stop() = 0;

signals:
	void stateChanged(Tw::LanguageServices::LanguageService::State state);
	void readinessChanged(bool ready);
	void capabilitiesChanged(const Tw::LanguageServices::LanguageServiceCapabilities & capabilities);
	void generationChanged(quint64 generation);
	void failed(const QString & reason);

protected:
	bool beginStart();
	void setState(State state);
	void becomeReady(const LanguageServiceCapabilities & capabilities);
	void becomeFailed(const QString & reason);
	void becomeStopped();

private:
	void clearCapabilities();

	State m_state{Stopped};
	LanguageServiceCapabilities m_capabilities;
	quint64 m_generation{0};
	QString m_failureReason;
};

} // namespace LanguageServices
} // namespace Tw

Q_DECLARE_METATYPE(Tw::LanguageServices::LanguageService::State)

#endif // LANGUAGESERVICE_H
