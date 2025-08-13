#ifndef SpellChecker_H
#define SpellChecker_H

#include <memory>
#include <QString>
#include <QTextCodec>

struct Hunhandle;

namespace Tw {
namespace Document {

class SpellChecker {
	friend class SpellCheckManager;

	QString _language;
	mutable std::weak_ptr<Hunhandle> _hunhandle;
	QTextCodec * _codec{QTextCodec::codecForLocale()};

	using DictType = std::shared_ptr<Hunhandle>;
	DictType getDict() const;

public:
	SpellChecker() = default;

	SpellChecker(const QString & language);

	bool operator==(const SpellChecker & other) const;
	bool operator!=(const SpellChecker & other) const { return !operator==(other); }
	operator bool() const { return isValid(); }

	bool isValid() const;

	QString getLanguage() const { return _language; }
	bool setLanguage(const QString & language);
	bool isWordCorrect(const QString & word) const;
	QList<QString> suggestionsForWord(const QString & word) const;
	// note that this is not persistent after quitting TW
	void ignoreWord(const QString & word);
};

} // namespace Document
} // namespace Tw

#endif // SpellChecker_H
