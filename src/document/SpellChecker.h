#ifndef SpellChecker_H
#define SpellChecker_H

#include <QString>
#include <QTextCodec>

struct Hunhandle;

namespace Tw {
namespace Document {

class SpellChecker {
	friend class SpellCheckManager;

	QString _language;
	Hunhandle * _hunhandle;
	QTextCodec * _codec;

	SpellChecker(const QString & language, Hunhandle * hunhandle);
public:
	virtual ~SpellChecker();
	QString getLanguage() const { return _language; }
	bool isWordCorrect(const QString & word) const;
	QList<QString> suggestionsForWord(const QString & word) const;
	// note that this is not persistent after quitting TW
	void ignoreWord(const QString & word);
};

} // namespace Document
} // namespace Tw

#endif // SpellChecker_H
