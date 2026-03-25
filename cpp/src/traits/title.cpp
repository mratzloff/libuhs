#include "uhs/traits/title.h"

namespace UHS::Traits {

Title::Title(std::string const& title) : title_{title} {}

std::string const& Title::title() const {
	return title_;
}

void Title::title(std::string const& title) {
	title_ = title;
}

} // namespace UHS::Traits
