#if !defined UHS_TRAITS_TITLE_H
#define UHS_TRAITS_TITLE_H

#include <string>

namespace UHS {
namespace Traits {

class Title {
public:
	Title() = default;
	explicit Title(std::string const& title);
	std::string const& title() const;
	void title(std::string const& title);

private:
	std::string title_;
};

} // namespace Traits
} // namespace UHS

#endif
