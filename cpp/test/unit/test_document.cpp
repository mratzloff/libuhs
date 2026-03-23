#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

namespace UHS {

TEST_CASE("Document::create", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	REQUIRE(document->isDocument());
	REQUIRE(document->version() == VersionType::Version96a);
}

TEST_CASE("Document::versionString", "[document]") {
	REQUIRE(Document::create(VersionType::Version88a)->versionString() == "88a");
	REQUIRE(Document::create(VersionType::Version91a)->versionString() == "91a");
	REQUIRE(Document::create(VersionType::Version95a)->versionString() == "95a");
	REQUIRE(Document::create(VersionType::Version96a)->versionString() == "96a");
}

TEST_CASE("Document::isVersion", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	REQUIRE(document->isVersion(VersionType::Version96a));
	REQUIRE_FALSE(document->isVersion(VersionType::Version88a));
}

TEST_CASE("Document::version setter", "[document]") {
	auto document = Document::create(VersionType::Version88a);
	document->version(VersionType::Version96a);
	REQUIRE(document->version() == VersionType::Version96a);
}

TEST_CASE("Document::validChecksum", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	REQUIRE_FALSE(document->validChecksum());

	document->validChecksum(true);
	REQUIRE(document->validChecksum());
}

TEST_CASE("Document::find locates elements by ID", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 42);
	auto element3 = Element::create(ElementType::Text, 100);

	document->appendChild(element1);
	element1->appendChild(element2);
	element1->appendChild(element3);

	REQUIRE(document->find(1) == element1.get());
	REQUIRE(document->find(42) == element2.get());
	REQUIRE(document->find(100) == element3.get());
	REQUIRE(document->find(999) == nullptr);
}

TEST_CASE("Document::find updates after appendChild", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	REQUIRE(document->find(5) == nullptr);

	auto element = Element::create(ElementType::Hint, 5);
	document->appendChild(element);
	REQUIRE(document->find(5) == element.get());
}

TEST_CASE("Document::find updates after removeChild and reindex", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto element = Element::create(ElementType::Hint, 5);
	document->appendChild(element);

	REQUIRE(document->find(5) == element.get());
	document->removeChild(element);

	// removeChild detaches the parent before didRemove runs, so
	// findDocument() returns null and the index is not updated.
	// A reindex is needed to reflect the removal.
	document->reindex();
	REQUIRE(document->find(5) == nullptr);
}

TEST_CASE("Document::reindex rebuilds index", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 2);

	document->appendChild(element1);
	element1->appendChild(element2);

	document->reindex();

	REQUIRE(document->find(1) == element1.get());
	REQUIRE(document->find(2) == element2.get());
}

TEST_CASE("Document::clone produces independent copy", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Test Document");
	document->validChecksum(true);

	auto element = Element::create(ElementType::Subject, 1);
	element->title("Subject");
	document->appendChild(element);

	auto cloned = document->clone();
	REQUIRE(cloned->version() == VersionType::Version96a);
	REQUIRE(cloned->title() == "Test Document");
	REQUIRE(cloned->validChecksum());
	REQUIRE(cloned->numChildren() == 1);
	REQUIRE_FALSE(cloned->hasParent());

	// Cloned document has its own index
	auto found = cloned->find(1);
	REQUIRE(found != nullptr);
	REQUIRE(found != element.get()); // Different object

	// Modifications don't affect original
	cloned->title("Changed");
	REQUIRE(document->title() == "Test Document");
}

TEST_CASE("Document traits", "[document]") {
	auto document = Document::create(VersionType::Version96a);

	document->title("Game Hints");
	REQUIRE(document->title() == "Game Hints");

	document->visibility(VisibilityType::RegisteredOnly);
	REQUIRE(document->visibility() == VisibilityType::RegisteredOnly);

	document->attr("author", "test");
	REQUIRE(document->attr("author").value() == "test");
}

TEST_CASE("Document::normalize is no-op with no metadata", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto subject = Element::create(ElementType::Subject, 1);
	document->appendChild(subject);

	document->normalize();

	REQUIRE(document->numChildren() == 1);
	REQUIRE(document->firstChild().get() == subject.get());
}

TEST_CASE("Document::normalize preserves correct metadata order", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto subject = Element::create(ElementType::Subject, 1);
	document->appendChild(subject);

	auto version = Element::create(ElementType::Version);
	version->title("96a");
	document->appendChild(version);

	auto incentive = Element::create(ElementType::Incentive);
	document->appendChild(incentive);

	auto info = Element::create(ElementType::Info);
	document->appendChild(info);

	document->normalize();

	REQUIRE(document->numChildren() == 4);
	auto node = document->firstChild();
	REQUIRE(node.get() == subject.get());
	node = node->nextSibling();
	REQUIRE(node.get() == version.get());
	node = node->nextSibling();
	REQUIRE(node.get() == incentive.get());
	node = node->nextSibling();
	REQUIRE(node.get() == info.get());
	REQUIRE_FALSE(node->hasNextSibling());
}

TEST_CASE("Document::normalize creates info when incentive has no info", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto subject = Element::create(ElementType::Subject, 1);
	document->appendChild(subject);

	auto version = Element::create(ElementType::Version);
	version->title("96a");
	document->appendChild(version);

	auto incentive = Element::create(ElementType::Incentive);
	document->appendChild(incentive);

	document->normalize();

	REQUIRE(document->numChildren() == 4);

	auto last = document->lastChild();
	REQUIRE(static_cast<Element const&>(*last).elementType() == ElementType::Info);

	auto prev = last->previousSibling();
	REQUIRE(static_cast<Element const&>(*prev).elementType() == ElementType::Incentive);

	auto prevPrev = prev->previousSibling();
	REQUIRE(static_cast<Element const&>(*prevPrev).elementType() == ElementType::Version);
}

TEST_CASE("Document::normalize reorders metadata from wrong order", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto subject = Element::create(ElementType::Subject, 1);
	document->appendChild(subject);

	// Append in wrong order: info, incentive, version
	auto info = Element::create(ElementType::Info);
	document->appendChild(info);

	auto incentive = Element::create(ElementType::Incentive);
	document->appendChild(incentive);

	auto version = Element::create(ElementType::Version);
	version->title("96a");
	document->appendChild(version);

	document->normalize();

	REQUIRE(document->numChildren() == 4);
	auto node = document->firstChild();
	REQUIRE(node.get() == subject.get());
	node = node->nextSibling();
	REQUIRE(node.get() == version.get());
	node = node->nextSibling();
	REQUIRE(node.get() == incentive.get());
	node = node->nextSibling();
	REQUIRE(node.get() == info.get());
}

TEST_CASE("Document::normalize is idempotent", "[document]") {
	auto document = Document::create(VersionType::Version96a);
	auto subject = Element::create(ElementType::Subject, 1);
	document->appendChild(subject);

	// Start in wrong order
	auto info = Element::create(ElementType::Info);
	document->appendChild(info);

	auto incentive = Element::create(ElementType::Incentive);
	document->appendChild(incentive);

	auto version = Element::create(ElementType::Version);
	version->title("96a");
	document->appendChild(version);

	document->normalize();
	document->normalize();

	REQUIRE(document->numChildren() == 4);
	auto node = document->firstChild();
	REQUIRE(node.get() == subject.get());
	node = node->nextSibling();
	REQUIRE(node.get() == version.get());
	node = node->nextSibling();
	REQUIRE(node.get() == incentive.get());
	node = node->nextSibling();
	REQUIRE(node.get() == info.get());
}

} // namespace UHS
