#pragma once

#include "helpers.h"

#include <functional>
#include <string>
#include <vector>

namespace UHS {

struct BuilderEntry {
	std::function<std::shared_ptr<Document>()> builder;
	std::string filename;
	bool preserve = false;
};

std::vector<BuilderEntry> const& allBuilders();

// Formatting builders
std::shared_ptr<Document> buildMixedMonospaceHintDocument();
std::shared_ptr<Document> buildMonospaceOverflowTextDocument();
std::shared_ptr<Document> buildMonospaceTextDocument();
std::shared_ptr<Document> buildOverflowTextDocument();
std::shared_ptr<Document> buildPlainHyperlinkPlainTextDocument();
std::shared_ptr<Document> buildPlainMonospacePlainTextDocument();
std::shared_ptr<Document> buildPlainTextDocument();

// Media builders
std::shared_ptr<Document> buildGifaDocument();
std::shared_ptr<Document> buildHyperpngLinkDocument();
std::shared_ptr<Document> buildHyperpngOverlayDocument();
std::shared_ptr<Document> buildHyperpngSimpleDocument();
std::shared_ptr<Document> buildSoundDocument();

// Metadata builders
std::shared_ptr<Document> buildCommentDocument();
std::shared_ptr<Document> buildCreditDocument();
std::shared_ptr<Document> buildDocumentAttributesDocument();
std::shared_ptr<Document> buildIncentiveDocument();
std::shared_ptr<Document> buildVersionMetadataDocument();

// Structure builders
std::shared_ptr<Document> buildBlankElementsDocument();
std::shared_ptr<Document> buildBreakNodesDocument();
std::shared_ptr<Document> buildCrossLinksDocument();
std::shared_ptr<Document> buildDeepNesthintDocument();
std::shared_ptr<Document> buildMultipleHintsDocument();

// Version builders
std::shared_ptr<Document> build88aDocument();
std::shared_ptr<Document> build91aDocument();
std::shared_ptr<Document> build95aDocument();
std::shared_ptr<Document> build96aDocument();

} // namespace UHS
