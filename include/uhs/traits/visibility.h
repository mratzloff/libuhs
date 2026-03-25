#if !defined UHS_TRAITS_VISIBILITY_H
#define UHS_TRAITS_VISIBILITY_H

#include <string>

#include "uhs/constants.h"

namespace UHS {
namespace Traits {

class Visibility {
public:
	VisibilityType visibility() const;
	void visibility(VisibilityType visibility);
	std::string const visibilityString() const;

private:
	VisibilityType visibility_ = VisibilityType::All;
};

} // namespace Traits
} // namespace UHS

#endif
