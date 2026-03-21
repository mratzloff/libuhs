#include "uhs.h"

namespace UHS::Traits {

VisibilityType Visibility::visibility() const {
	return visibility_;
}

void Visibility::visibility(VisibilityType visibility) {
	visibility_ = visibility;
}

std::string const Visibility::visibilityString() const {
	switch (visibility_) {
	case VisibilityType::All:
		return "all";
	case VisibilityType::None:
		return "none";
	case VisibilityType::RegisteredOnly:
		return "registered";
	case VisibilityType::UnregisteredOnly:
		return "unregistered";
	default:
		throw DataError("invalid visibility");
	}
}

} // namespace UHS::Traits
