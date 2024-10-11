#pragma once

#include "CoreMinimal.h"

namespace Lexer
{
	enum class ETokenType : int32
	{
		String,
		Label,
		Say,
		Identifier,
		Number,
		Comment,
		Whitespace,
		NewLine,

		Count,
		Invalid = -1
	};

	const TCHAR* GetTokenTypeName(ETokenType AType);

	struct FToken
	{
		ETokenType Type = ETokenType::Invalid;
		FString Value = FString();
		int LineNumber = 0;
		int ColumnNumber = 0;

		bool IsValid() const { return Type != ETokenType::Invalid && !Value.IsEmpty(); }
	};

	struct FSentence
	{
	private:
		TArray<FToken> Tokens;

	public:
		FSentence() = default;
		FSentence(const TArray<FToken>& ATokens);

		int32 GetLineNumber() const;
		const FString& GetKeyword() const;

		void Reset();
		bool IsValid() const;
		bool IsComment() const;
		bool IsLabel() const;

		bool IsKeywordSay() const;
		bool KeywordIs(const FString& AName) const;

		bool TryGetStringAtIndex(int32 AIndex, FString& AValue) const;
		bool TryGetStringProperty(const FString& AName, FString& AValue) const;

		bool TryGetIntAtIndex(int32 AIndex, int32& AValue) const;
		bool TryGetIntProperty(const FString& AName, int32& AValue) const;

		bool TryGetDoubleAtIndex(int32 AIndex, double& AValue) const;
		bool TryGetDoubleProperty(const FString& AName, double& AValue) const;

		bool Contains(const FString& AName) const;

	private:

		bool TryGetIndexOf(const FString& AName, int32& AIndex) const;
	};

	bool TryTokenize(const FString& AInput, TArray<FSentence>& ASentences);
}
