#include "uhs.h"

namespace UHS::Traits {

Inlined::Inlined(bool inlined) : inlined_{inlined} {}

bool Inlined::inlined() const {
	return inlined_;
}

void Inlined::inlined(bool inlined) {
	inlined_ = inlined;
}

} // namespace UHS::Traits
