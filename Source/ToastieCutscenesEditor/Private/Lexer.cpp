#include "Lexer.h"

namespace Lexer
{
	const TCHAR* GetTokenTypeName(ETokenType AType)
	{
		switch (AType)
		{
		case ETokenType::String:		return TEXT("String");
		case ETokenType::Label:			return TEXT("Label");
		case ETokenType::Say:			return TEXT("Say");
		case ETokenType::Identifier:	return TEXT("Identifier");
		case ETokenType::Number:		return TEXT("Number");
		case ETokenType::Comment:		return TEXT("Comment");
		case ETokenType::Whitespace:	return TEXT("Whitespace");
		case ETokenType::NewLine:		return TEXT("NewLine");
		default:						return TEXT("Unknown");
		}
	}

	struct FTokenPatternEntry
	{
		ETokenType Type;
		FRegexPattern Pattern;

		const FTokenPatternEntry(ETokenType AType, const TCHAR* APattern)
			: Type(AType)
			, Pattern(APattern) { }
	};

	static const TArray<FTokenPatternEntry> TokenPatterns =
	{
		FTokenPatternEntry(ETokenType::String,		TEXT("(\")(.*)(\")")),
		FTokenPatternEntry(ETokenType::Label,		TEXT("(\\[)[A-Za-z_\\-/0-9]+(\\])")),
		FTokenPatternEntry(ETokenType::Say,			TEXT("[A-Za-z_\\-/0-9]+:")),
		FTokenPatternEntry(ETokenType::Number,		TEXT("[0-9]*\\.?[0-9]+")),
		FTokenPatternEntry(ETokenType::Identifier,	TEXT("[A-Za-z_\\-/0-9]+")),
		FTokenPatternEntry(ETokenType::Comment,		TEXT(";(.*)")),
		FTokenPatternEntry(ETokenType::Whitespace,	TEXT("[ \t]+")),
		FTokenPatternEntry(ETokenType::NewLine,		TEXT("(\r\n|\r|\n)"))
	};

	FSentence::FSentence(const TArray<FToken>& ATokens)
		: Tokens()
	{
		for (int I = 0; I < ATokens.Num(); ++I)
		{
			Tokens.Add(ATokens[I]);
		}
	}

	int32 FSentence::GetLineNumber() const
	{
		return IsValid() ? Tokens[0].LineNumber : -1;
	}

	const FString& FSentence::GetKeyword() const
	{
		return Tokens[0].Value;
	}

	void FSentence::Reset()
	{
		Tokens.Reset();
	}

	bool FSentence::IsValid() const
	{
		return Tokens.Num() > 0 && Tokens[0].IsValid();
	}

	bool FSentence::IsComment() const
	{
		return IsValid() && Tokens[0].Type == ETokenType::Comment;
	}

	bool FSentence::IsLabel() const
	{
		return IsValid() && Tokens[0].Type == ETokenType::Label;
	}

	bool FSentence::IsKeywordSay() const
	{
		return IsValid() && Tokens[0].Value.EndsWith(TEXT(":"));
	}

	bool FSentence::KeywordIs(const FString& AName) const
	{
		return IsValid() && Tokens[0].Value == AName;
	}

	bool FSentence::TryGetStringAtIndex(int32 AIndex, FString& AValue) const
	{
		if (Tokens.IsValidIndex(AIndex))
		{
			AValue = Tokens[AIndex].Value;
			return true;
		}
		return false;
	}

	bool FSentence::TryGetStringProperty(const FString& AName, FString& AValue) const
	{
		int32 Index;
		return TryGetIndexOf(AName, Index) && TryGetStringAtIndex(Index + 1, AValue);
	}

	bool FSentence::TryGetIntAtIndex(int32 AIndex, int32& AValue) const
	{
		if (Tokens.IsValidIndex(AIndex))
		{
			auto& ValueString = Tokens[AIndex].Value;
			if (FCString::IsNumeric(*ValueString))
			{
				AValue = FCString::Atoi(*ValueString);
				return true;
			}
		}
		AValue = 0;
		return false;
	}

	bool FSentence::TryGetIntProperty(const FString& AName, int32& AValue) const
	{
		int32 Index;
		return TryGetIndexOf(AName, Index) && TryGetIntAtIndex(Index + 1, AValue);
	}

	bool FSentence::TryGetDoubleAtIndex(int32 AIndex, double& AValue) const
	{
		if (Tokens.IsValidIndex(AIndex))
		{
			auto& ValueString = Tokens[AIndex].Value;
			if (FCString::IsNumeric(*ValueString))
			{
				AValue = FCString::Atod(*ValueString);
				return true;
			}
		}
		AValue = 0.0;
		return false;
	}

	bool FSentence::TryGetDoubleProperty(const FString& AName, double& AValue) const
	{
		int32 Index;
		return TryGetIndexOf(AName, Index) && TryGetDoubleAtIndex(Index + 1, AValue);
	}

	bool FSentence::Contains(const FString& AName) const
	{
		int32 Index;
		return TryGetIndexOf(AName, Index);
	}

	bool FSentence::TryGetIndexOf(const FString& AName, int32& AIndex) const
	{
		for (int32 I = 0; I < Tokens.Num(); ++I)
		{
			if (Tokens[I].Value == AName)
			{
				AIndex = I;
				return true;
			}
		}
		AIndex = -1;
		return false;
	}

	bool TryTokenize(const FString& AInput, TArray<FSentence>& ASentences)
	{
		struct FTokenMatcher
		{
			const FTokenPatternEntry& PatternEntry;
			FRegexMatcher Matcher;

			FTokenMatcher(const FTokenPatternEntry& APatternEntry, const FString& AInput)
				: PatternEntry(APatternEntry)
				, Matcher(APatternEntry.Pattern, AInput) { }
		};

		ASentences.Reset();
		ASentences.Add(FSentence());

		// Create matchers for each token pattern
		TArray<FTokenMatcher> TokenMatchers;
		for (auto& TokenPattern : TokenPatterns)
		{
			TokenMatchers.Add(FTokenMatcher(TokenPattern, AInput));
		}

		TArray<FToken> CurrentTokens;
		int32 LineNumber = 1;
		int32 ColumnNumber = 1;

		int32 Current = 0;
		while (Current < AInput.Len())
		{
			auto bMatchFound = false;

			// Find the first matching pattern
			for (auto& TokenMatcher : TokenMatchers)
			{
				TokenMatcher.Matcher.SetLimits(Current, AInput.Len());

				if (TokenMatcher.Matcher.FindNext() && TokenMatcher.Matcher.GetMatchBeginning() - Current == 0)
				{
					auto MatchBegin = TokenMatcher.Matcher.GetMatchBeginning();
					auto MatchLen = TokenMatcher.Matcher.GetMatchEnding() - MatchBegin;

					Current += MatchLen;

					if (TokenMatcher.PatternEntry.Type != ETokenType::Whitespace &&
						TokenMatcher.PatternEntry.Type != ETokenType::NewLine)
					{
						const auto MatchValue = AInput.Mid(MatchBegin, MatchLen);
						CurrentTokens.Add(FToken(TokenMatcher.PatternEntry.Type, MatchValue, LineNumber, ColumnNumber));
					}

					if (TokenMatcher.PatternEntry.Type == ETokenType::NewLine)
					{
						ColumnNumber = 1;
						LineNumber += 1;

						ASentences.Last() = FSentence(CurrentTokens);
						if (ASentences.Last().IsValid())
							ASentences.Add(FSentence());

						CurrentTokens.Reset();
					}
					else
					{
						ColumnNumber += MatchLen;
					}

					bMatchFound = true;
					break;
				}
			}

			// If no match was found, we error out
			if (!bMatchFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCS: Unable to find match"));
				return false;
			}
		}

		return true;
	}
}