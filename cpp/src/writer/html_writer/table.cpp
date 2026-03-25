#include "uhs.h"

namespace UHS {

HTMLWriter::Table::Table(std::vector<std::string> const& lines) : lines_{lines} {}

std::size_t HTMLWriter::Table::endLine() const {
	return endLine_;
}

bool HTMLWriter::Table::hasPrecedingText() const {
	return startLine_ > 0;
}

void HTMLWriter::Table::parse() {
	endLine_ = lines_.size();

	if (lines_.empty()) {
		valid_ = false;
		return;
	}

	demarcationLine_ = findDemarcationLine();

	Boundaries columnBoundaries;
	std::size_t dataStartLine = 0;

	if (demarcationLine_ > 0) {
		// Find the start of the contiguous header block above the demarcation
		startLine_ = demarcationLine_ - 1;
		while (startLine_ > 0 && !lines_[startLine_ - 1].empty()) {
			--startLine_;
		}

		// Bail if there are non-empty lines before the header block
		for (std::size_t i = 0; i < startLine_; ++i) {
			if (!lines_[i].empty()) {
				valid_ = false;
				return;
			}
		}

		if (parsePipeDelimitedTable()) {
			return;
		}

		columnBoundaries = detectColumnBoundaries();
		if (columnBoundaries.empty()) {
			valid_ = false;
			return;
		}

		charGrid_ = detectCharGrid(columnBoundaries);
		addHeaderRows(columnBoundaries);
		dataStartLine = demarcationLine_ + 1;
	} else {
		columnBoundaries = detectHeaderlessColumnBoundaries();
		if (columnBoundaries.empty()) {
			valid_ = false;
			return;
		}

		headerless_ = true;
		startLine_ = 0;
		dataStartLine = 0;
	}

	int expectedNumColumns = columnBoundaries.size();

	// Table width from demarcation line (used to detect line wrapping)
	std::size_t tableWidth = 0;
	if (demarcationLine_ >= 0 && demarcationLine_ < static_cast<int>(lines_.size())) {
		tableWidth = lines_[demarcationLine_].size();
	}

	// Add data rows
	bool afterSeparator = false;
	for (auto it = lines_.begin() + dataStartLine; it < lines_.end(); ++it) {
		auto const& line = *it;

		if (line.empty() || Strings::trim(line, ' ').empty()) {
			afterSeparator = true;
			continue;
		}

		if (isNextTableBoundary(it + 1)) {
			endLine_ = static_cast<std::size_t>(it - lines_.begin());
			break;
		}

		auto naturalBounds = detectBoundariesFromLine(line);

		if (isContinuationLine(
		        line, naturalBounds, columnBoundaries, tableWidth, afterSeparator)) {
			mergeContinuationLine(line, naturalBounds, columnBoundaries);
			afterSeparator = false;
			continue;
		}

		afterSeparator = false;
		rows_.push_back(extractDataRowCells(
		    line, naturalBounds, columnBoundaries, expectedNumColumns));
	}

	valid_ = true;
}

void HTMLWriter::Table::serialize(pugi::xml_node& xmlNode) const {
	xmlNode.append_attribute("class");
	xmlNode.attribute("class") = "table-container";

	auto table = xmlNode.append_child("table");
	table.append_attribute("class");
	table.attribute("class") = "option option-html hidden";

	if (!headerless_) {
		serializeHeader(table);
	}

	serializeBody(table);
	serializeTextFallback(xmlNode);
}

std::size_t HTMLWriter::Table::startLine() const {
	return startLine_;
}

bool HTMLWriter::Table::valid() const {
	return valid_;
}

void HTMLWriter::Table::addHeaderRows(Boundaries const& columnBoundaries) {

	for (auto it = lines_.begin() + demarcationLine_ - 1; it >= lines_.begin(); --it) {
		auto const& line = *it;

		if (!line.empty()) {
			auto cells = extractCellsByBoundaries(line, columnBoundaries);
			rows_.insert(rows_.begin(), cells);
		}
	}
}

HTMLWriter::Table::Boundaries HTMLWriter::Table::detectBoundariesFromLine(
    std::string const& line) const {

	Boundaries boundaries;

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

bool HTMLWriter::Table::detectCharGrid(Boundaries const& columnBoundaries) const {

	if (columnBoundaries.size() < 2) {
		return false;
	}

	for (auto const& [start, end] : columnBoundaries) {
		if (end - start != 1 || (start > 0 && start % 2 != 0)) {
			return false;
		}
	}

	return true;
}

HTMLWriter::Table::Boundaries HTMLWriter::Table::detectColumnBoundaries() const {

	if (demarcationLine_ <= 0) {
		return {};
	}

	auto const& header = lines_[demarcationLine_ - 1];
	auto const& demarcation = lines_[demarcationLine_];
	auto headerBoundaries = detectBoundariesFromLine(header);
	auto demarcationBoundaries = detectBoundariesFromLine(demarcation);

	// Prefer demarcation boundaries (full column width) over header boundaries
	// (text width only), but fall back to header if the demarcation has fewer
	// segments.
	Boundaries boundaries;
	if (demarcationBoundaries.size() >= headerBoundaries.size()) {
		boundaries = demarcationBoundaries;
	} else {
		boundaries = headerBoundaries;
	}

	// Fallback: single-character grid (e.g. "1 2 3 4 5" over "---------")
	if (boundaries.size() == 1 && header.size() >= 3 && header.size() % 2 == 1) {
		bool isCharGrid = true;
		for (std::size_t i = 0; i < header.size(); ++i) {
			if ((i % 2 == 0) ? (header[i] == ' ') : (header[i] != ' ')) {
				isCharGrid = false;
				break;
			}
		}
		if (isCharGrid) {
			boundaries.clear();
			for (std::size_t i = 0; i < header.size(); i += 2) {
				boundaries.emplace_back(i, i + 1);
			}
		}
	}

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
		Boundaries refined;
		std::size_t dataIndex = 0;

		for (auto const& [headerStart, headerEnd] : boundaries) {
			// Collect data column starts that fall within this header column
			std::vector<std::size_t> dataStarts;
			while (dataIndex < dataBoundaries.size()
			       && dataBoundaries[dataIndex].first < headerEnd) {
				dataStarts.push_back(dataBoundaries[dataIndex].first);
				++dataIndex;
			}

			if (dataStarts.size() <= 1) {
				refined.emplace_back(headerStart, headerEnd);
				continue;
			}

			// Split this header column at each extra data column start
			auto previousStart = headerStart;
			for (std::size_t s = 1; s < dataStarts.size(); ++s) {
				// Search backward from the data start for a space in the header
				auto pos = dataStarts[s];
				while (pos > previousStart && pos < header.size() && header[pos] != ' ') {
					--pos;
				}

				if (pos > previousStart && pos < header.size()) {
					refined.emplace_back(previousStart, pos);

					// Skip spaces to find next column start
					previousStart = pos;
					while (previousStart < headerEnd && previousStart < header.size()
					       && header[previousStart] == ' ') {
						++previousStart;
					}
				} else if (dataStarts[s] >= header.size()) {
					// Header doesn't extend to this data column — split
					// at the data column start
					refined.emplace_back(previousStart, dataStarts[s]);
					previousStart = dataStarts[s];
				}
			}
			refined.emplace_back(previousStart, headerEnd);
		}

		boundaries = refined;
		break;
	}

	return boundaries;
}

HTMLWriter::Table::Boundaries
    HTMLWriter::Table::detectHeaderlessColumnBoundaries() const {

	// Count how many non-continuation lines produce each column count
	std::map<std::size_t, int> columnFreqs;
	int numDataLines = 0;

	for (auto const& line : lines_) {
		if (line.empty() || std::isspace(static_cast<unsigned char>(line[0]))) {
			continue;
		}
		auto bounds = detectBoundariesFromLine(line);
		++columnFreqs[bounds.size()];
		++numDataLines;
	}

	if (numDataLines < 3) {
		return {};
	}

	// Find most common column count >= 2
	std::size_t numConsensusColumns = 0;
	int consensusFreq = 0;
	for (auto const& [numColumns, freq] : columnFreqs) {
		if (numColumns >= 2 && freq > consensusFreq) {
			numConsensusColumns = numColumns;
			consensusFreq = freq;
		}
	}

	auto const consensusRatio =
	    static_cast<double>(consensusFreq) / static_cast<double>(numDataLines);
	if (numConsensusColumns < 2 || consensusRatio < HeaderlessConsensusThreshold) {
		return {};
	}

	// Compute consensus boundaries by averaging start positions across matching lines
	std::vector<double> startSums(numConsensusColumns, 0.0);
	std::vector<double> endSums(numConsensusColumns, 0.0);
	int numMatches = 0;

	for (auto const& line : lines_) {
		if (line.empty() || std::isspace(static_cast<unsigned char>(line[0]))) {
			continue;
		}
		auto bounds = detectBoundariesFromLine(line);
		if (bounds.size() != numConsensusColumns) {
			continue;
		}
		for (std::size_t c = 0; c < numConsensusColumns; ++c) {
			startSums[c] += bounds[c].first;
			endSums[c] += bounds[c].second;
		}
		++numMatches;
	}

	Boundaries boundaries;
	for (std::size_t c = 0; c < numConsensusColumns; ++c) {
		auto start = static_cast<std::size_t>(startSums[c] / numMatches + 0.5);
		auto end = static_cast<std::size_t>(endSums[c] / numMatches + 0.5);
		boundaries.emplace_back(start, end);
	}

	return boundaries;
}

std::vector<std::string> HTMLWriter::Table::extractCellsByBoundaries(
    std::string const& line, Boundaries const& boundaries) const {

	// Adjust boundaries when data values extend past header column positions
	std::vector<std::size_t> columnStarts;
	for (std::size_t b = 0; b < boundaries.size(); ++b) {
		auto start = boundaries[b].first;
		if (b > 0 && start > 0 && start < line.size() && line[start] != ' '
		    && line[start - 1] != ' ') {
			// Search left for nearest space
			auto leftPos = start;
			while (leftPos > 0 && line[leftPos - 1] != ' ') {
				--leftPos;
			}

			// Search right for nearest space
			auto rightPos = start;
			while (rightPos < line.size() && line[rightPos] != ' ') {
				++rightPos;
			}

			if (start - leftPos <= rightPos - start) {
				start = leftPos;
			} else {
				start = rightPos;
			}
		}
		columnStarts.push_back(start);
	}

	std::vector<std::string> cells;
	for (std::size_t b = 0; b < columnStarts.size(); ++b) {
		auto columnStart = columnStarts[b];

		std::size_t columnEnd;
		if (b + 1 < columnStarts.size()) {
			columnEnd = columnStarts[b + 1];
		} else {
			columnEnd = line.size();
		}

		if (columnStart < line.size()) {
			auto len = std::min(columnEnd, line.size()) - columnStart;
			cells.push_back(Strings::trim(line.substr(columnStart, len), ' '));
		} else {
			cells.push_back("");
		}
	}
	return cells;
}

std::vector<std::string> HTMLWriter::Table::extractDataRowCells(std::string const& line,
    Boundaries const& naturalBounds, Boundaries const& columnBoundaries,
    int expectedNumColumns) const {

	if (charGrid_ || static_cast<int>(naturalBounds.size()) >= expectedNumColumns - 1) {
		return extractCellsByBoundaries(line, columnBoundaries);
	}

	// Non-tabular text — extract using natural boundaries
	std::vector<std::string> cells;
	for (auto const& [start, end] : naturalBounds) {
		cells.push_back(Strings::trim(line.substr(start, end - start), ' '));
	}
	while (static_cast<int>(cells.size()) < expectedNumColumns) {
		cells.push_back("");
	}

	return cells;
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

bool HTMLWriter::Table::isContinuationLine(std::string const& line,
    Boundaries const& naturalBounds, Boundaries const& columnBoundaries,
    std::size_t tableWidth, bool afterSeparator) const {

	if (rows_.empty() || afterSeparator || !std::isspace(line[0])
	    || naturalBounds.size() > 2) {
		return false;
	}

	if (tableWidth == 0) {
		return true;
	}

	if (charGrid_) {
		return false;
	}

	// Each segment's text combined with the previous row's cell must exceed
	// the column width (indicating text overflowed the column rather than
	// being a new row with empty leading columns)
	auto const& lastRow = rows_.back();
	for (auto const& [segStart, segEnd] : naturalBounds) {
		auto text = Strings::trim(line.substr(segStart, segEnd - segStart), ' ');
		if (text.empty()) {
			continue;
		}

		std::size_t column = columnBoundaries.size() - 1;
		for (std::size_t c = 0; c + 1 < columnBoundaries.size(); ++c) {
			if (segStart < columnBoundaries[c + 1].first) {
				column = c;
				break;
			}
		}

		std::size_t columnWidth;
		if (column + 1 < columnBoundaries.size()) {
			columnWidth =
			    columnBoundaries[column + 1].first - columnBoundaries[column].first;
		} else {
			columnWidth = tableWidth - columnBoundaries[column].first;
		}

		if (column >= lastRow.size()
		    || lastRow[column].size() + 1 + text.size() <= columnWidth) {
			return false;
		}
	}

	return true;
}

bool HTMLWriter::Table::isNextTableBoundary(
    std::vector<std::string>::const_iterator start) const {

	for (auto peek = start; peek < lines_.end(); ++peek) {
		if (peek->empty() || Strings::trim(*peek, ' ').empty()) {
			continue;
		}
		std::smatch match;
		return std::regex_match(*peek, match, Regex::HorizontalLine);
	}

	return false;
}

void HTMLWriter::Table::mergeContinuationLine(std::string const& line,
    Boundaries const& naturalBounds, Boundaries const& columnBoundaries) {

	auto& lastRow = rows_.back();
	for (auto const& [segmentStart, segmentEnd] : naturalBounds) {
		auto text =
		    Strings::trim(line.substr(segmentStart, segmentEnd - segmentStart), ' ');
		if (text.empty()) {
			continue;
		}

		// Find which column this segment falls into
		std::size_t column = columnBoundaries.size() - 1;
		for (std::size_t c = 0; c + 1 < columnBoundaries.size(); ++c) {
			if (segmentStart < columnBoundaries[c + 1].first) {
				column = c;
				break;
			}
		}

		if (column < lastRow.size()) {
			if (!lastRow[column].empty()) {
				lastRow[column] += " " + text;
			} else {
				lastRow[column] = text;
			}
		}
	}
}

bool HTMLWriter::Table::parsePipeDelimitedTable() {
	auto const& header = lines_[demarcationLine_ - 1];
	if (std::count(header.begin(), header.end(), '|') < 3) {
		return false;
	}

	auto splitPipe = [](std::string const& line) {
		std::vector<std::string> cells;
		std::size_t start = 0;
		while (start <= line.size()) {
			auto pos = line.find('|', start);
			if (pos == std::string::npos) {
				pos = line.size();
			}
			cells.push_back(Strings::trim(line.substr(start, pos - start), ' '));
			start = pos + 1;
		}
		return cells;
	};

	auto headerCells = splitPipe(header);
	auto expectedPipes = std::count(header.begin(), header.end(), '|');

	rows_.push_back(headerCells);
	pipeDelimited_ = true;

	for (auto i = demarcationLine_ + 1; i < static_cast<int>(lines_.size()); ++i) {
		auto const& line = lines_[i];
		if (line.empty() || Strings::trim(line, ' ').empty()) {
			continue;
		}
		if (std::count(line.begin(), line.end(), '|') < expectedPipes / 2) {
			endLine_ = i;
			break;
		}
		auto cells = splitPipe(line);
		while (cells.size() < headerCells.size()) {
			cells.push_back("");
		}
		rows_.push_back(cells);
	}

	valid_ = true;
	return true;
}

void HTMLWriter::Table::serializeBody(pugi::xml_node& table) const {
	auto tbody = table.append_child("tbody");
	auto tbodyStart = headerless_ ? rows_.begin() : rows_.begin() + demarcationLine_;
	for (auto it = tbodyStart; it < rows_.end(); ++it) {
		auto tr = tbody.append_child("tr");
		auto const& row = *it;
		for (auto const& cell : row) {
			auto td = tr.append_child("td");
			td.append_child(pugi::node_pcdata).set_value(cell.c_str());
		}
	}
}

void HTMLWriter::Table::serializeHeader(pugi::xml_node& table) const {
	auto thead = table.append_child("thead");
	for (auto it = rows_.begin(); it < rows_.begin() + demarcationLine_; ++it) {
		auto const& row = *it;
		auto tr = thead.append_child("tr");

		int nonEmptyCount = 0;
		for (auto const& cell : row) {
			if (!cell.empty()) {
				++nonEmptyCount;
			}
		}

		if (nonEmptyCount == 0) {
			thead.remove_child(tr);
			continue;
		}

		if (nonEmptyCount == 1 && row.size() > 1) {
			for (auto const& cell : row) {
				if (!cell.empty()) {
					auto th = tr.append_child("th");
					th.append_attribute("colspan") = static_cast<int>(row.size());
					th.append_child(pugi::node_pcdata).set_value(cell.c_str());
					break;
				}
			}
		} else {
			for (auto const& cell : row) {
				auto th = tr.append_child("th");
				th.append_child(pugi::node_pcdata).set_value(cell.c_str());
			}
		}
	}
}

void HTMLWriter::Table::serializeTextFallback(pugi::xml_node& parent) const {
	auto text = parent.append_child("div");
	text.append_attribute("class");
	text.attribute("class") = "option option-text monospace overflow";

	for (std::size_t i = 0; i < endLine_; ++i) {
		if (i > 0) {
			text.append_child("br");
		}
		text.append_child(pugi::node_pcdata).set_value(lines_[i].c_str());
	}
}

} // namespace UHS
