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
			auto cells = splitLine(line);
			// Pad with empty cells if needed
			while (static_cast<int>(cells.size()) < expectedNumColumns) {
				cells.push_back("");
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

		auto cells = splitLine(line);

		// Check if this is a continuation line
		auto isContinuation = false;
		if (!line.empty() && std::isspace(line[0])) {
			// Only treat a line as a continuation if it has 1-2 cells
			if (static_cast<int>(cells.size()) <= 2) {
				isContinuation = true;
			}
		}

		if (!table_.empty() && isContinuation) {
			// For continuation lines, split into cells and assign each to correct column
			processContinuationLine(line, columnBoundaries);
		} else {
			// This is a new row; pad with empty cells if needed
			while (static_cast<int>(cells.size()) < expectedNumColumns) {
				cells.push_back("");
			}
			table_.push_back(cells);
		}
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
    HTMLWriter::Table::detectColumnBoundaries() const {

	std::vector<std::pair<std::size_t, std::size_t>> boundaries;

	if (demarcationLine_ <= 0) {
		return boundaries;
	}

	auto const header = lines_[demarcationLine_ - 1];
	auto maxLength = header.size();
	if (static_cast<std::size_t>(demarcationLine_ + 1) < lines_.size()) {
		maxLength = std::max(maxLength, lines_[demarcationLine_ + 1].size());
	}

	std::vector<bool> isSpaceGap(maxLength, false);

	// Mark positions that are parts of 2+ space gap
	auto spaceCount = 0;
	for (std::size_t i = 0; i <= header.size(); ++i) {
		if (i < header.size() && header[i] == ' ') {
			spaceCount++;
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

	for (std::size_t i = 0; i < header.size(); ++i) {
		auto isGap = isSpaceGap[i];

		if (isGap && inColumn) {
			boundaries.emplace_back(columnStart, i);
			inColumn = false;
		} else if (!isGap && !inColumn) {
			columnStart = i;
			inColumn = true;
		}
	}

	if (inColumn) {
		boundaries.emplace_back(columnStart, header.size());
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

// Helper method to process continuation lines
void HTMLWriter::Table::processContinuationLine(std::string const& line,
    std::vector<std::pair<std::size_t, std::size_t>> const& columnBoundaries) {

	if (table_.empty()) {
		return;
	}

	auto const& cells = splitLine(line);
	auto& lastRow = table_.back();

	if (!cells.empty()) {
		// Find the position of each cell in the original line
		std::size_t searchStart = 0;
		for (auto const& cell : cells) {
			// Find this cell's position in the original line
			std::size_t cellPos = line.find(cell, searchStart);
			if (cellPos != std::string::npos) {
				// Determine which column this position falls into
				for (std::size_t col = 0; col < columnBoundaries.size(); ++col) {
					auto const& [colStart, colEnd] = columnBoundaries[col];
					if (cellPos >= colStart && cellPos < colEnd) {
						// Append to this column
						if (col < lastRow.size()) {
							if (!lastRow[col].empty()) {
								lastRow[col] += " " + cell;
							} else {
								lastRow[col] = cell;
							}
						}
						break;
					}
				}
				// Move search position forward for next cell
				searchStart = cellPos + cell.length();
			}
		}
	}
}

// Splits a line by 2+ consecutive spaces or space-minus-digit pattern
std::vector<std::string> HTMLWriter::Table::splitLine(std::string const& line) const {
	std::vector<std::string> cells;
	std::string currentCell;
	int spaceCount = 0;

	for (std::size_t i = 0; i < line.size(); ++i) {
		char c = line[i];
		if (c == ' ') {
			spaceCount++;
			continue;
		}

		// Non-space character
		if (spaceCount >= 2) {
			// This is a column boundary
			currentCell = Strings::trim(currentCell, ' ');
			if (!currentCell.empty()) {
				cells.push_back(currentCell);
			}
			spaceCount = 0;
			currentCell.clear();
			currentCell += c;
		} else if (spaceCount == 1 && c == '-') {
			// Special case: space-minus might be column boundary
			// Only if minus is followed by a digit (i.e., a negative number)
			bool isNegativeNumber = (i + 1 < line.size() && std::isdigit(line[i + 1]));
			if (isNegativeNumber) {
				// This is space-minus-digit, treat as boundary
				currentCell = Strings::trim(currentCell, ' ');
				if (!currentCell.empty()) {
					cells.push_back(currentCell);
				}
				spaceCount = 0;
				currentCell.clear();
				currentCell += c; // Add the minus to new cell
			} else {
				// Space-minus without digit is part of cell (e.g., "- none -")
				spaceCount = 0;
				currentCell += ' ';
				currentCell += c;
			}
		} else if (spaceCount == 1) {
			// Single space within a cell (not before minus); add it
			spaceCount = 0;
			currentCell += ' ';
			currentCell += c;
		} else {
			// No preceding spaces
			currentCell += c;
		}
	}

	// Handle remaining spaces and cell
	if (!currentCell.empty()) {
		currentCell = Strings::trim(currentCell, ' ');
		if (!currentCell.empty()) {
			cells.push_back(currentCell);
		}
	}

	return cells;
}

} // namespace UHS
