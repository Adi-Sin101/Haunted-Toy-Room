#include "PuzzleCode.h"

#include <algorithm>

void PuzzleCode::Reset()
{
	discovered.fill(false);
	digits.clear();
	complete = false;
}

void PuzzleCode::Discover(Clue clue)
{
	discovered[static_cast<size_t>(clue)] = true;
}

bool PuzzleCode::Found(Clue clue) const
{
	return discovered[static_cast<size_t>(clue)];
}

bool PuzzleCode::AllCluesFound() const
{
	return std::all_of(discovered.begin(), discovered.end(), [](bool found) { return found; });
}

int PuzzleCode::CluesFound() const
{
	return static_cast<int>(std::count(discovered.begin(), discovered.end(), true));
}

bool PuzzleCode::AddDigit(int digit)
{
	if (digit < 0 || digit > 9 || digits.size() >= 3 || complete) return false;
	digits.push_back(static_cast<char>('0' + digit));
	return true;
}

bool PuzzleCode::Backspace()
{
	if (digits.empty() || complete) return false;
	digits.pop_back();
	return true;
}

PuzzleCode::Result PuzzleCode::Submit()
{
	if (complete) return Result::Correct;
	if (digits.size() != 3) return Result::Incomplete;
	if (!AllCluesFound()) {
		digits.clear();
		return Result::MissingClues;
	}
	if (digits == "257") {
		complete = true;
		return Result::Correct;
	}
	digits.clear();
	return Result::Incorrect;
}

std::string PuzzleCode::Display() const
{
	std::string result = "CODE: ";
	for (size_t i = 0; i < 3; ++i) {
		if (i) result += ' ';
		result += i < digits.size() ? digits[i] : '_';
	}
	return result;
}
