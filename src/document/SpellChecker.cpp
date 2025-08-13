#include "SpellChecker.h"

#include "SpellCheckManager.h"

#include <hunspell.h>

namespace Tw {
namespace Document {

std::shared_ptr<Hunhandle> SpellChecker::getDict() const
{
	DictType dict = _hunhandle.lock();
	if (!dict) {
		dict = SpellCheckManager::getDictionary(_language);
		_hunhandle = dict;
	}
	return dict;
}

SpellChecker::SpellChecker(const QString & language)
	: SpellChecker()
{
	setLanguage(language);
}

bool SpellChecker::operator==(const SpellChecker & other) const
{
	return (_language == other._language && getDict() == other.getDict() && _codec == other._codec);
}

bool SpellChecker::isValid() const
{
	return (!_language.isEmpty() && getDict());
}

bool SpellChecker::setLanguage(const QString & language)
{
	const DictType & dict{SpellCheckManager::getDictionary(language)};
	_hunhandle = dict;

	if (!dict) {
		_language.clear();
		_codec = QTextCodec::codecForLocale();
		return (language.isEmpty() == true);
	}

	_language = language;
	_codec = QTextCodec::codecForName(Hunspell_get_dic_encoding(dict.get()));
	if (_codec == nullptr) {
		_codec = QTextCodec::codecForLocale();
	}
	return true;
}

bool SpellChecker::isWordCorrect(const QString & word) const
{
	const DictType & dict{getDict()};
	if (!dict || _codec == nullptr) {
		return false;
	}
	return (Hunspell_spell(dict.get(), _codec->fromUnicode(word).data()) != 0);
}

QList<QString> SpellChecker::suggestionsForWord(const QString & word) const
{
	const DictType & dict{getDict()};
	if (!dict || _codec == nullptr) {
		return {};
	}
	QList<QString> suggestions;
	char ** suggestionList{nullptr};

	int numSuggestions = Hunspell_suggest(dict.get(), &suggestionList, _codec->fromUnicode(word).data());
	suggestions.reserve(numSuggestions);
	for (int iSuggestion = 0; iSuggestion < numSuggestions; ++iSuggestion)
		suggestions.append(_codec->toUnicode(suggestionList[iSuggestion]));

	Hunspell_free_list(dict.get(), &suggestionList, numSuggestions);

	return suggestions;
}

void SpellChecker::ignoreWord(const QString & word)
{
	const DictType & dict{getDict()};
	if (!dict || _codec == nullptr) {
		return;
	}
	// note that this is not persistent after quitting TW
	Hunspell_add(dict.get(), _codec->fromUnicode(word).data());
}

} // namespace Document
} // namespace Tw
