#if !defined UHS_TRAITS_ATTRIBUTES_H
#define UHS_TRAITS_ATTRIBUTES_H

#include <map>
#include <optional>
#include <string>

namespace UHS {
namespace Traits {

class Attributes {
public:
	using Type = std::map<std::string, std::string>;

	Type const& attrs() const;
	std::optional<std::string const> attr(std::string const& key) const;
	void attr(std::string const& key, std::string const& value);
	void attr(std::string const& key, int const value);

private:
	Type attrs_;
};

} // namespace Traits
} // namespace UHS

#endif
