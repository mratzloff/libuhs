#if !defined UHS_TRAITS_INLINED_H
#define UHS_TRAITS_INLINED_H

namespace UHS {
namespace Traits {

class Inlined { // `inline` is a C++ keyword
public:
	Inlined() = default;
	explicit Inlined(bool inlined);
	bool inlined() const;
	void inlined(bool inlined);

private:
	bool inlined_ = false;
};

} // namespace Traits
} // namespace UHS

#endif
