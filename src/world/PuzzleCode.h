#pragma once

#include <array>
#include <string>

// Small, render-independent rules for the three clue and three digit challenge.
class PuzzleCode {
public:
	enum class Clue : int { Train, Clock, Blocks };
	enum class Result { Incomplete, MissingClues, Incorrect, Correct };

	void Reset();
	void Discover(Clue clue);
	bool Found(Clue clue) const;
	bool AllCluesFound() const;
	int CluesFound() const;
	bool AddDigit(int digit);
	bool Backspace();
	Result Submit();
	std::string Display() const;
	const std::string& Digits() const { return digits; }
	bool Complete() const { return complete; }

private:
	std::array<bool, 3> discovered{};
	std::string digits;
	bool complete = false;
};
