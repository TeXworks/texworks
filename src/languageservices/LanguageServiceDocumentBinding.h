/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICEDOCUMENTBINDING_H
#define LANGUAGESERVICEDOCUMENTBINDING_H

#include "languageservices/LanguageService.h"

#include <QPointer>

namespace Tw {
namespace Document { class TeXDocument; }
namespace LanguageServices {

class LanguageServiceDocumentBinding : public QObject
{
	Q_OBJECT

public:
	LanguageServiceDocumentBinding(Document::TeXDocument * document, LanguageService * service, QObject * parent = nullptr);

	void updateIdentity(const QUrl & url, const QString & languageId);
	void prepareServiceStop();
	void close();
	bool canRequestCompletion() const;
	bool requestCompletion(const LanguagePosition & position);
	void cancelCompletionRequest();
	void beginCompletionEdit();
	void endCompletionEdit();
	bool canRequestDefinition() const;
	bool requestDefinition(const LanguagePosition & position);
	void cancelDefinitionRequest();

	QUrl url() const { return m_url; }
	QString languageId() const { return m_languageId; }
	quint64 version() const { return m_version; }
	quint64 identityGeneration() const { return m_identityGeneration; }
	quint64 serviceGeneration() const { return m_serviceGeneration; }
	bool isSynchronized() const { return m_synchronized; }
	bool isOpenOnServer() const { return m_openOnServer; }
	QString shadow() const { return m_shadow; }

signals:
	void completionAvailabilityChanged(bool available);
	void completionReady(const QList<Tw::LanguageServices::CompletionItem> & items);
	void definitionAvailabilityChanged(bool available);
	void definitionReady(const QList<Tw::LanguageServices::LanguageLocation> & locations);

private:
	static LanguagePosition positionInText(const QString & text, int offset);
	void handleContentsChange(int position, int charsRemoved, int charsAdded);
	void handleServiceState(LanguageService::State state);
	void handleCompletionResult(quint64 token, const QList<CompletionItem> & items);
	void handleDefinitionResult(quint64 token, const QList<LanguageLocation> & locations);
	void activate();
	void deactivate(bool notifyClose);
	void updateCompletionAvailability();
	void updateDefinitionAvailability();
	bool isPositionValid(const LanguagePosition & position) const;

	QPointer<Document::TeXDocument> m_document;
	QPointer<LanguageService> m_service;
	QUrl m_url;
	QString m_languageId;
	QString m_shadow;
	quint64 m_version{0};
	quint64 m_identityGeneration{0};
	quint64 m_serviceGeneration{0};
	quint64 m_completionToken{0};
	quint64 m_completionServiceGeneration{0};
	quint64 m_completionIdentityGeneration{0};
	quint64 m_completionVersion{0};
	quint64 m_definitionToken{0};
	quint64 m_definitionServiceGeneration{0};
	quint64 m_definitionIdentityGeneration{0};
	quint64 m_definitionVersion{0};
	bool m_synchronized{false};
	bool m_openOnServer{false};
	bool m_closed{false};
	bool m_completionAvailable{false};
	bool m_definitionAvailable{false};
	int m_completionEditDepth{0};
};

} // namespace LanguageServices
} // namespace Tw

#endif // LANGUAGESERVICEDOCUMENTBINDING_H
