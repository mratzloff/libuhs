#include "uhs.h"

namespace UHS {

HTMLWriter::Table::Table(std::vector<std::string> const& lines) : lines_{lines} {}

void HTMLWriter::Table::parse() {
	endLine_ = lines_.size();

	if (lines_.empty()) {
		valid_ = false;
		return;
	}

	demarcationLine_ = findDemarcationLine();
	if (demarcationLine_ <= 0) {
		valid_ = false;
		return;
	}

	// Detect column boundaries from header row
	auto columnBoundaries = detectColumnBoundaries();
	if (columnBoundaries.empty()) {
		valid_ = false;
		return;
	}

	int expectedNumColumns = columnBoundaries.size();

	// Add header rows (all non-empty lines before demarcation)
	for (auto it = lines_.begin() + demarcationLine_ - 1; it >= lines_.begin(); --it) {
		auto const& line = *it;

		if (!line.empty()) {
			// Use column boundaries to extract header cells
			std::vector<std::string> cells;
			for (std::size_t b = 0; b < columnBoundaries.size(); ++b) {
				auto colStart = columnBoundaries[b].first;
				auto colEnd = (b + 1 < columnBoundaries.size())
				    ? columnBoundaries[b + 1].first
				    : line.size();
				if (colStart < line.size()) {
					auto len = std::min(colEnd, line.size()) - colStart;
					cells.push_back(Strings::trim(line.substr(colStart, len), ' '));
				} else {
					cells.push_back("");
				}
			}
			table_.insert(table_.begin(), cells);
		}
	}

	// Add data rows (all lines after demarcation)
	for (auto it = lines_.begin() + demarcationLine_ + 1; it < lines_.end(); ++it) {
		auto const& line = *it;

		if (line.empty()) {
			continue;
		}

		auto naturalBounds = detectBoundariesFromLine(line);

		// Continuation line: starts with space, few segments
		if (!table_.empty() && std::isspace(line[0])
		    && naturalBounds.size() <= 2) {
			auto& lastRow = table_.back();
			for (auto const& [segStart, segEnd] : naturalBounds) {
				auto text = Strings::trim(
				    line.substr(segStart, segEnd - segStart), ' ');
				if (text.empty()) {
					continue;
				}

				// Find which column this segment falls into
				std::size_t col = columnBoundaries.size() - 1;
				for (std::size_t c = 0; c + 1 < columnBoundaries.size(); ++c) {
					if (segStart < columnBoundaries[c + 1].first) {
						col = c;
						break;
					}
				}

				if (col < lastRow.size()) {
					if (!lastRow[col].empty()) {
						lastRow[col] += " " + text;
					} else {
						lastRow[col] = text;
					}
				}
			}
			continue;
		}

		// Extract cells
		std::vector<std::string> cells;
		if (static_cast<int>(naturalBounds.size()) >= expectedNumColumns - 1) {
			// Normal row or single-space gap — use column boundaries
			for (std::size_t b = 0; b < columnBoundaries.size(); ++b) {
				auto colStart = columnBoundaries[b].first;
				auto colEnd = (b + 1 < columnBoundaries.size())
				    ? columnBoundaries[b + 1].first
				    : line.size();
				if (colStart < line.size()) {
					auto len = std::min(colEnd, line.size()) - colStart;
					cells.push_back(
					    Strings::trim(line.substr(colStart, len), ' '));
				} else {
					cells.push_back("");
				}
			}
		} else {
			// Non-tabular text — extract using natural boundaries
			for (auto const& [start, end] : naturalBounds) {
				cells.push_back(
				    Strings::trim(line.substr(start, end - start), ' '));
			}
			while (static_cast<int>(cells.size()) < expectedNumColumns) {
				cells.push_back("");
			}
		}

		table_.push_back(cells);
	}

	// Exclude last row if it's a single long cell (not really table data)
	if (table_.size() > static_cast<std::size_t>(demarcationLine_) + 1) {
		auto const& lastRow = table_.back();
		auto nonEmptyCount = 0;
		std::size_t longestLength = 0;

		for (auto const& cell : lastRow) {
			if (!cell.empty()) {
				++nonEmptyCount;
				longestLength = std::max(longestLength, cell.size());
			}
		}

		if (nonEmptyCount == 1 && longestLength > SuspectCellLength) {
			table_.pop_back();

			// Find the corresponding line in lines_
			for (auto i = static_cast<int>(lines_.size()) - 1; i > demarcationLine_;
			    --i) {
				if (!lines_[i].empty()) {
					endLine_ = i;
					break;
				}
			}
		}
	}

	valid_ = true;
}

void HTMLWriter::Table::serialize(pugi::xml_node& xmlNode) const {
	auto tableContainer = xmlNode.append_child("div");
	tableContainer.append_attribute("class");
	tableContainer.attribute("class") = "table-container";

	auto table = tableContainer.append_child("table");
	table.append_attribute("class");
	table.attribute("class") = "option option-html hidden";

	auto thead = table.append_child("thead");
	for (auto it = table_.begin(); it < table_.begin() + demarcationLine_; ++it) {
		auto const& row = *it;
		auto tr = thead.append_child("tr");
		for (auto const& cell : row) {
			auto th = tr.append_child("th");
			th.append_child(pugi::node_pcdata).set_value(cell.c_str());
		}
	}

	auto tbody = table.append_child("tbody");
	for (auto it = table_.begin() + demarcationLine_; it < table_.end(); ++it) {
		auto tr = tbody.append_child("tr");
		auto const& row = *it;
		for (auto const& cell : row) {
			auto td = tr.append_child("td");
			td.append_child(pugi::node_pcdata).set_value(cell.c_str());
		}
	}

	auto text = tableContainer.append_child("div");
	text.append_attribute("class");
	text.attribute("class") = "option option-text monospace overflow";

	for (std::size_t i = 0; i < endLine_; ++i) {
		if (i > 0) {
			text.append_child("br");
		}
		text.append_child(pugi::node_pcdata).set_value(lines_[i].c_str());
	}
}

std::size_t HTMLWriter::Table::endLine() const {
	return endLine_;
}

bool HTMLWriter::Table::valid() const {
	return valid_;
}

std::vector<std::pair<std::size_t, std::size_t>>
    HTMLWriter::Table::detectBoundariesFromLine(std::string const& line) const {

	std::vector<std::pair<std::size_t, std::size_t>> boundaries;

	// Mark positions that are parts of 2+ space gap
	std::vector<bool> isSpaceGap(line.size(), false);
	auto spaceCount = 0;
	for (std::size_t i = 0; i <= line.size(); ++i) {
		if (i < line.size() && line[i] == ' ') {
			++spaceCount;
		} else {
			if (spaceCount >= 2) {
				for (std::size_t j = i - spaceCount; j < i; ++j) {
					isSpaceGap[j] = true;
				}
			}
			spaceCount = 0;
		}
	}

	// Extract columns
	auto inColumn = false;
	std::size_t columnStart = 0;

	for (std::size_t i = 0; i < line.size(); ++i) {
		if (isSpaceGap[i] && inColumn) {
			boundaries.emplace_back(columnStart, i);
			inColumn = false;
		} else if (!isSpaceGap[i] && !inColumn) {
			columnStart = i;
			inColumn = true;
		}
	}

	if (inColumn) {
		boundaries.emplace_back(columnStart, line.size());
	}

	return boundaries;
}

std::vector<std::pair<std::size_t, std::size_t>>
    HTMLWriter::Table::detectColumnBoundaries() const {

	if (demarcationLine_ <= 0) {
		return {};
	}

	auto const& header = lines_[demarcationLine_ - 1];
	auto boundaries = detectBoundariesFromLine(header);

	// Refine using the first non-empty data row
	for (auto i = demarcationLine_ + 1; i < static_cast<int>(lines_.size()); ++i) {
		if (lines_[i].empty()) {
			continue;
		}

		auto dataBoundaries = detectBoundariesFromLine(lines_[i]);
		if (dataBoundaries.size() <= boundaries.size()) {
			break;
		}

		// Map each data column start to the header column it falls within.
		// Split header columns that contain 2+ data column starts.
		std::vector<std::pair<std::size_t, std::size_t>> refined;
		std::size_t dataIndex = 0;

		for (auto const& [hStart, hEnd] : boundaries) {
			// Collect data column starts that fall within this header column
			std::vector<std::size_t> dataStarts;
			while (dataIndex < dataBoundaries.size()
			       && dataBoundaries[dataIndex].first < hEnd) {
				dataStarts.push_back(dataBoundaries[dataIndex].first);
				++dataIndex;
			}

			if (dataStarts.size() <= 1) {
				refined.emplace_back(hStart, hEnd);
				continue;
			}

			// Split this header column at each extra data column start
			auto prevStart = hStart;
			for (std::size_t s = 1; s < dataStarts.size(); ++s) {
				// Search backward from the data start for a space in the header
				auto splitPos = dataStarts[s];
				while (splitPos > prevStart && splitPos < header.size()
				       && header[splitPos] != ' ') {
					--splitPos;
				}

				if (splitPos > prevStart && splitPos < header.size()) {
					refined.emplace_back(prevStart, splitPos);

					// Skip spaces to find next column start
					prevStart = splitPos;
					while (prevStart < hEnd && prevStart < header.size()
					       && header[prevStart] == ' ') {
						++prevStart;
					}
				}
			}
			refined.emplace_back(prevStart, hEnd);
		}

		boundaries = refined;
		break;
	}

	return boundaries;
}

int HTMLWriter::Table::findDemarcationLine() const {
	std::smatch match;
	for (std::size_t i = 0; i < lines_.size(); ++i) {
		if (std::regex_match(lines_[i], match, Regex::HorizontalLine)) {
			return static_cast<int>(i);
		}
	}
	return -1;
}


} // namespace UHS
