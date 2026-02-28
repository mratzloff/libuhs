#include "uhs.h"

namespace UHS {

void HTMLWriter::Table::boostFirst(std::vector<WordAggregate>& aggregates) {
	for (auto& aggregate : aggregates) {
		aggregate.score = std::pow(aggregate.score, 10);
		break;
	}
}

void HTMLWriter::Table::boostLeadingSpaces(std::vector<WordAggregate>& aggregates) {
	for (auto& aggregate : aggregates) {
		aggregate.leadingSpacesScore = std::pow(aggregate.leadingSpacesScore, 2);
	}
}

int HTMLWriter::Table::calcNumColumns(std::vector<WordAggregate>& aggregates) {
	if (aggregates.size() <= 1) {
		return aggregates.empty() ? 0 : 1;
	}

	// Calculate mean (excluding first aggregate)
	double mean = 0.0;
	for (auto it = aggregates.begin() + 1; it != aggregates.end(); ++it) {
		mean += it->score;
	}
	mean /= (aggregates.size() - 1);

	// Calculate standard deviation (excluding first aggregate)
	double variance = 0.0;
	for (auto it = aggregates.begin() + 1; it != aggregates.end(); ++it) {
		double diff = it->score - mean;
		variance += diff * diff;
	}
	variance /= (aggregates.size() - 1);
	double stdDev = std::sqrt(variance);

	// Count entries with score > 1 std dev from mean (excluding first aggregate)
	int count = 0;
	for (auto it = aggregates.begin() + 1; it != aggregates.end(); ++it) {
		if (std::abs(it->score - mean) > stdDev) {
			++count;
		}
	}

	return count + 1; // Add 1 for the first aggregate
}

void HTMLWriter::Table::mergeAdjacentIndexes(std::vector<WordAggregate>& aggregates) {
	std::sort(aggregates.begin(), aggregates.end(), [](auto const& a, auto const& b) {
		return b.index < a.index;
	});

	WordAggregate* previous = nullptr;
	auto it = aggregates.begin();
	while (it != aggregates.end()) {
		auto& current = *it;

		if (previous && current.index == previous->index - 1
		    && previous->maxLeadingSpaces > 1) {
			// Merge previous into current
			current.indexFrequency += previous->indexFrequency;
			current.maxLeadingSpaces =
			    std::max(current.maxLeadingSpaces, previous->maxLeadingSpaces);

			// Erase previous (which is at it - 1)
			it = aggregates.erase(std::prev(it));
			previous = &current;
		} else {
			previous = &current;
			++it;
		}
	}
}

void HTMLWriter::Table::normalizeIndexFrequency(std::vector<WordAggregate>& aggregates) {

	auto max = 0;
	auto min = INT_MAX;

	for (auto const& aggregate : aggregates) {
		if (aggregate.indexFrequency < min) {
			min = aggregate.indexFrequency;
		}
		if (aggregate.indexFrequency > max) {
			max = aggregate.indexFrequency;
		}
	}

	for (auto& aggregate : aggregates) {
		if (max == min) {
			aggregate.indexFrequencyScore = 1.0;
		} else {
			tfm::printf("[%d) min:%d, max:%d, freq:%d, score:%d]\n",
			    aggregate.index,
			    min,
			    max,
			    aggregate.indexFrequency,
			    (static_cast<double>(aggregate.indexFrequency) - static_cast<double>(min))
			        / static_cast<double>(max - min));
			aggregate.indexFrequencyScore =
			    (static_cast<double>(aggregate.indexFrequency) - static_cast<double>(min))
			    / static_cast<double>(max - min);
		}
	}
}

void HTMLWriter::Table::normalizeLeadingSpaces(std::vector<WordAggregate>& aggregates) {

	auto max = 0;
	auto min = INT_MAX;

	for (auto const& aggregate : aggregates) {
		if (aggregate.maxLeadingSpaces < min) {
			min = aggregate.maxLeadingSpaces;
		}
		if (aggregate.maxLeadingSpaces > max) {
			max = aggregate.maxLeadingSpaces;
		}
	}

	for (auto& aggregate : aggregates) {
		if (aggregate.maxLeadingSpaces == 1) {
			aggregate.leadingSpacesScore = 0.0;
		} else if (max == min) {
			aggregate.leadingSpacesScore = 1.0;
		} else {
			aggregate.leadingSpacesScore =
			    (static_cast<double>(aggregate.maxLeadingSpaces)
			        - static_cast<double>(min))
			    / static_cast<double>(max - min);
		}
	}
}

void HTMLWriter::Table::score(std::vector<WordAggregate>& aggregates) {
	for (auto& aggregate : aggregates) {
		aggregate.score = (IndexFrequencyWeight * aggregate.indexFrequency)
		                  + (LeadingSpacesWeight * aggregate.maxLeadingSpaces);
	}
}

void HTMLWriter::Table::parse(std::vector<std::string> lines) {
	auto foundHeader = false;
	auto numColumns = 0;
	auto numHeadings = 0;
	auto numHeadingRows = 0;
	auto numTrailingEmptyLines = 0;
	std::vector<std::vector<Word>> wordsByRow;

	tfm::printf("== BEGIN table =====================\n");
	for (auto sit = lines.cbegin(); sit < lines.cend(); ++sit) {
		tfm::printf(">%s\n", *sit);
	}
	tfm::printf("== END =============================\n");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::smatch match;

		tfm::printf("\"%s\"\n", lines[i]);
		if (!foundHeader && std::regex_match(lines[i], match, Regex::HorizontalLine)) {
			auto const matches = Strings::split(lines[i], std::regex{"\\s+"});
			int const numHorizontalRules = matches.size();

			foundHeader = true;
			numHeadingRows = i + 1;

			if (numHorizontalRules == 0) { // False positive
				continue;
			}
			if (numHorizontalRules > 1) {
				numColumns = numHorizontalRules;
				continue;
			}

			auto const headings = Strings::split(lines[i - 1], std::regex{"\\s{2,}"});
			numHeadings = headings.size();
		}

		auto inWord = false;
		auto lineEnd = lines[i].size();
		auto numSpaces = 0; // Leading spaces before word
		std::vector<Word> words;

		for (std::size_t j = 0; j < lineEnd; ++j) {
			if (lines[i][j] != ' ' && !inWord) {
				// We've entered a word
				words.emplace_back(j, numSpaces);
				inWord = true;
			}
			if (j + 1 < lineEnd && lines[i][j] == ' ' && inWord) {
				// We've exited a word
				inWord = false;
				numSpaces = 0;
			}
			if (lines[i][j] == ' ') {
				++numSpaces;
			}
		}

		std::cout << "[index -> spaces = ";
		for (auto word : words) {
			tfm::printf("[%d, %d], ", word.index, word.numLeadingSpaces);
		}
		std::cout << "]\n";

		if (words.size() > 0) {
			numTrailingEmptyLines = 0;
		} else {
			++numTrailingEmptyLines;
		}

		wordsByRow.emplace_back(words);
	}

	tfm::printf("[numColumns = %d]\n\n", numColumns);

	// Next, determine the most likely column boundaries

	tsl::hopscotch_map<std::size_t, WordAggregate> aggregateMap;
	for (auto const& words : wordsByRow) {
		for (auto const& word : words) {
			auto& aggregate = aggregateMap[word.index];

			aggregate.index = word.index;
			++aggregate.indexFrequency;

			if (aggregate.minLeadingSpaces == 0) {
				aggregate.minLeadingSpaces = INT_MAX;
			}
			if (word.numLeadingSpaces > aggregate.maxLeadingSpaces) {
				aggregate.maxLeadingSpaces = word.numLeadingSpaces;
			}
			if (word.numLeadingSpaces < aggregate.minLeadingSpaces) {
				aggregate.minLeadingSpaces = word.numLeadingSpaces;
			}
		}
	}

	std::vector<WordAggregate> aggregates;
	for (auto it = aggregateMap.begin(); it != aggregateMap.end(); ++it) {
		aggregates.push_back(it.value());
	}

	// For debugging; remove after this is working
	std::sort(aggregates.begin(), aggregates.end(), [](auto const& a, auto const& b) {
		return a.index < b.index;
	});

	std::cout << "[index frequency = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.indexFrequency);
	}
	std::cout << "]\n";

	std::cout << "[max leading spaces = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.maxLeadingSpaces);
	}
	std::cout << "]\n";

	mergeAdjacentIndexes(aggregates);

	std::cout << "[merged indexes = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.indexFrequency);
	}
	std::cout << "]\n";

	normalizeIndexFrequency(aggregates);

	std::cout << "[normalized index frequency = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.indexFrequencyScore);
	}
	std::cout << "]\n";

	normalizeLeadingSpaces(aggregates);

	std::cout << "[normalized leading spaces = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.leadingSpacesScore);
	}
	std::cout << "]\n";

	boostLeadingSpaces(aggregates);

	std::cout << "[boosted leading spaces = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.leadingSpacesScore);
	}
	std::cout << "]\n";

	score(aggregates);

	std::cout << "[score = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.score);
	}
	std::cout << "]\n";

	std::sort(aggregates.begin(), aggregates.end(), [](auto const& a, auto const& b) {
		return a.index < b.index;
	});

	boostFirst(aggregates);

	std::cout << "[boosted score = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.score);
	}
	std::cout << "]\n";

	std::sort(aggregates.begin(), aggregates.end(), [](auto const& a, auto const& b) {
		return b.score < a.score;
	});

	std::cout << "[sorted score = ";
	for (auto const& aggregate : aggregates) {
		tfm::printf("%d (%d), ", aggregate.index, aggregate.score);
	}
	std::cout << "]\n";

	if (numColumns == 0) {
		numColumns = calcNumColumns(aggregates);
	}
	if (numHeadings > numColumns) {
		numColumns = numHeadings;
	}

	std::vector<std::size_t> columnIndexes;
	auto numWordIndexFreqs = static_cast<int>(aggregates.size());
	for (auto i = 0; i < numColumns && i < numWordIndexFreqs; ++i) {
		columnIndexes.push_back(aggregates[i].index);
	}
	std::sort(columnIndexes.begin(), columnIndexes.end());

	std::cout << "[columnIndexes = ";
	for (auto const& index : columnIndexes) {
		tfm::printf("%d, ", index);
	}
	std::cout << "]\n";

	// TODO: Check if any indexes are in the middle of header words
	// If so, remove those indexes from the columnIndexes

	std::vector<std::vector<std::string>> rows;
	std::size_t finalRowIndex = lines.size() - numTrailingEmptyLines;
	for (std::size_t i = 0; i < finalRowIndex; ++i) {
		std::vector<std::string> row;
		for (int j = 0; j < numColumns; ++j) {
			std::size_t end = std::string::npos;
			if (j + 1 < numColumns && lines[i].length() > columnIndexes[j + 1]) {
				end = columnIndexes[j + 1] - columnIndexes[j];
			}
			std::string cell;
			if (lines[i].length() > columnIndexes[j]) {
				tfm::printf("[substr %d %d] ", columnIndexes[j], end);
				cell = lines[i].substr(columnIndexes[j], end);
			}
			// TODO: Logic for continuations
			row.push_back(Strings::trim(cell, ' '));
		}
		std::cout << "\n";
		rows.push_back(row);
	}

	// for (auto const& heading : headings) {
	// 	tfm::printf("|%s", heading);
	// }
	// std::cout << "|\n";

	// for (auto i = 0; i < numColumns; ++i) {
	// 	std::cout << "|---";
	// }
	// std::cout << "|\n";

	for (auto const& row : rows) {
		for (auto const& cell : row) {
			tfm::printf("|%s", cell);
		}
		std::cout << "|\n";
	}
}

void HTMLWriter::Table::serialize(pugi::xml_node& xmlNode) {}

bool HTMLWriter::Table::valid() {
	return valid_;
}

} // namespace UHS
