#include "uhs.h"

namespace UHS {

std::vector<std::pair<std::size_t, std::size_t>>
    HTMLWriter::Table::detectColumnBoundaries(
        std::vector<std::string> const& lines, int demarcationIndex) {

	std::vector<std::pair<std::size_t, std::size_t>> boundaries;

	if (demarcationIndex <= 0) {
		return boundaries;
	}

	auto header = lines[demarcationIndex - 1];
	auto maxLength = header.size();
	if (static_cast<std::size_t>(demarcationIndex + 1) < lines.size()) {
		maxLength = std::max(maxLength, lines[demarcationIndex + 1].size());
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

int HTMLWriter::Table::findDemarcationLine(std::vector<std::string> const& lines) {
	for (std::size_t i = 0; i < lines.size(); ++i) {
		int dashCount = std::count(lines[i].begin(), lines[i].end(), '-');
		if (dashCount >= 4) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

void HTMLWriter::Table::parse(std::vector<std::string> lines) {
	if (lines.empty()) {
		valid_ = false;
		return;
	}

	tfm::printf("== BEGIN table =====================\n");
	for (auto sit = lines.cbegin(); sit < lines.cend(); ++sit) {
		tfm::printf(">%s\n", *sit);
	}
	tfm::printf("== END =============================\n");

	auto demarcationIndex = findDemarcationLine(lines);
	if (demarcationIndex <= 0) {
		valid_ = false;
		return;
	}

	// Detect column boundaries from header row
	auto columnBoundaries = detectColumnBoundaries(lines, demarcationIndex);
	if (columnBoundaries.empty()) {
		valid_ = false;
		return;
	}

	int expectedNumColumns = columnBoundaries.size();

	// Parse table rows
	std::vector<std::vector<std::string>> table;

	// Add header rows (all non-empty lines before demarcation)
	for (auto i = demarcationIndex - 1; i >= 0; --i) {
		if (!lines[i].empty()) {
			auto cells = splitBySpaces(lines[i]);
			// Pad with empty cells if needed
			while (static_cast<int>(cells.size()) < expectedNumColumns) {
				cells.push_back("");
			}
			table.insert(table.begin(), cells);
		}
	}

	// Add data rows (all lines after demarcation)
	for (std::size_t i = demarcationIndex + 1; i < lines.size(); ++i) {
		if (lines[i].empty()) {
			continue;
		}

		auto cells = splitBySpaces(lines[i]);

		// Check if this is a continuation line
		auto isContinuation = false;
		if (!lines[i].empty() && std::isspace(lines[i][0])) {
			// Only treat a line as a continuation if it has 1-2 cells
			if (static_cast<int>(cells.size()) <= 2) {
				isContinuation = true;
			}
		}

		if (!table.empty() && isContinuation) {
			// For continuation lines, split into cells and assign each to correct column
			processContinuationLine(lines[i], table, columnBoundaries);
		} else {
			// This is a new row; pad with empty cells if needed
			while (static_cast<int>(cells.size()) < expectedNumColumns) {
				cells.push_back("");
			}
			table.push_back(cells);
		}
	}

	// Print the parsed table
	for (auto const& row : table) {
		for (auto const& cell : row) {
			tfm::printf("|%s", cell);
		}
		std::cout << "|\n";
	}

	valid_ = true;
}

// Splits a line by 2+ consecutive spaces or space-minus-digit pattern
std::vector<std::string> HTMLWriter::Table::splitBySpaces(std::string const& line) {
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

// Helper method to process continuation lines
void HTMLWriter::Table::processContinuationLine(std::string const& line,
    std::vector<std::vector<std::string>>& table,
    std::vector<std::pair<std::size_t, std::size_t>> const& columnBoundaries) {

	if (table.empty()) {
		return;
	}

	auto cells = splitBySpaces(line);
	auto& lastRow = table.back();

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

void HTMLWriter::Table::serialize(pugi::xml_node& xmlNode) {}

bool HTMLWriter::Table::valid() {
	return valid_;
}

} // namespace UHS
