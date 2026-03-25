#if !defined UHS_TRAITS_BODY_H
#define UHS_TRAITS_BODY_H

#include <string>

namespace UHS {
namespace Traits {

class Body {
public:
	Body() = default;
	explicit Body(std::string const& body);
	explicit Body(int const body);
	std::string const& body() const;
	void body(std::string const& body);
	void body(int const body);

private:
	std::string body_;
};

} // namespace Traits
} // namespace UHS

#endif
