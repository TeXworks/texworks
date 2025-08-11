#include "SpellChecker.h"

#include <hunspell.h>

namespace Tw {
namespace Document {

SpellChecker::SpellChecker(const QString & language, Hunhandle * hunhandle)
	: _language(language)
	, _hunhandle(hunhandle)
	, _codec(nullptr)
{
	if (_hunhandle)
		_codec = QTextCodec::codecForName(Hunspell_get_dic_encoding(_hunhandle));
	if (!_codec)
		_codec = QTextCodec::codecForLocale(); // almost certainly wrong, if we couldn't find the actual name!
}

SpellChecker::~SpellChecker()
{
	if (_hunhandle)
		Hunspell_destroy(_hunhandle);
}

bool SpellChecker::isWordCorrect(const QString & word) const
{
	return (Hunspell_spell(_hunhandle, _codec->fromUnicode(word).data()) != 0);
}

QList<QString> SpellChecker::suggestionsForWord(const QString & word) const
{
	QList<QString> suggestions;
	char ** suggestionList{nullptr};

	int numSuggestions = Hunspell_suggest(_hunhandle, &suggestionList, _codec->fromUnicode(word).data());
	suggestions.reserve(numSuggestions);
	for (int iSuggestion = 0; iSuggestion < numSuggestions; ++iSuggestion)
		suggestions.append(_codec->toUnicode(suggestionList[iSuggestion]));

	Hunspell_free_list(_hunhandle, &suggestionList, numSuggestions);

	return suggestions;
}

void SpellChecker::ignoreWord(const QString & word)
{
	// note that this is not persistent after quitting TW
	Hunspell_add(_hunhandle, _codec->fromUnicode(word).data());
}

} // namespace Document
} // namespace Tw
