#ifndef SpellChecker_H
#define SpellChecker_H

#include <QObject>
#include <QHash>

struct Hunhandle;

namespace Tw {
namespace Document {

class SpellChecker : public QObject {
	Q_OBJECT

	SpellChecker() = default;
	virtual ~SpellChecker() = default;
	SpellChecker(const SpellChecker & other) = delete;
	SpellChecker & operator=(const SpellChecker & other) = delete;

public:
	class Dictionary {
		friend class SpellChecker;

		QString _language;
		Hunhandle * _hunhandle;
		QTextCodec * _codec;

		Dictionary(const QString & language, Hunhandle * hunhandle);
	public:
		virtual ~Dictionary();
		QString getLanguage() const { return _language; }
		bool isWordCorrect(const QString & word) const;
		QList<QString> suggestionsForWord(const QString & word) const;
		// note that this is not persistent after quitting TW
		void ignoreWord(const QString & word);
	};

	static SpellChecker * instance() { return _instance; }

	// get list of available dictionaries
	static QHash<QString, QString> * getDictionaryList(const bool forceReload = false);

	// get dictionary for a given language
	static Dictionary * getDictionary(const QString& language);
	// deallocates all dictionaries
	// WARNING: Don't call this while some window is using a dictionary as that
	// window won't be notified; deactivate spell checking in all windows first
	// (see TWApp::reloadSpellchecker())
	static void clearDictionaries();

signals:
	// emitted when getDictionaryList reloads the dictionary list;
	// windows can connect to it to rebuild, e.g., a spellchecking menu
	void dictionaryListChanged() const;

private:
	static SpellChecker * _instance;
	static QHash<QString, QString> * dictionaryList;
	static QHash<const QString,SpellChecker::Dictionary*> * dictionaries;
};

} // namespace Document
} // namespace Tw

#endif // !defined(SpellChecker_H)
