#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "uhs.h"

namespace UHS {

struct HTMLWriter::TableAccessor {
	using Boundaries = HTMLWriter::Table::Boundaries;

	HTMLWriter::Table& table;

	int demarcationLine() const { return table.demarcationLine_; }
	HTMLWriter::Table::Boundaries detectBoundariesFromLine(
	    std::string const& line) const {
		return table.detectBoundariesFromLine(line);
	}
	std::vector<std::string> extractCellsByBoundaries(
	    std::string const& line, HTMLWriter::Table::Boundaries const& boundaries) const {
		return table.extractCellsByBoundaries(line, boundaries);
	}
	int findDemarcationLine() const { return table.findDemarcationLine(); }
	bool isCharGrid() const { return table.charGrid_; }
	bool isHeaderless() const { return table.headerless_; }
	bool isPipeDelimited() const { return table.pipeDelimited_; }
	std::vector<std::vector<std::string>> const& rows() const { return table.rows_; }
};

// --- findDemarcationLine ---

TEST_CASE("Table::findDemarcationLine finds dashes", "[table]") {
	HTMLWriter::Table table({"Header", "----------", "data"});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == 1);
}

TEST_CASE("Table::findDemarcationLine returns -1 when absent", "[table]") {
	HTMLWriter::Table table({"just", "some", "text"});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == -1);
}

TEST_CASE("Table::findDemarcationLine finds first match", "[table]") {
	HTMLWriter::Table table({"header1", "------", "header2", "------", "data"});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == 1);
}

TEST_CASE("Table::findDemarcationLine ignores short dashes", "[table]") {
	HTMLWriter::Table table({"header", "--", "data"});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == -1);
}

TEST_CASE("Table::findDemarcationLine matches segmented dashes", "[table]") {
	HTMLWriter::Table table({"Col1  Col2", "----  ----", "a     b"});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == 1);
}

TEST_CASE("Table::findDemarcationLine with empty input", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.findDemarcationLine() == -1);
}

// --- detectBoundariesFromLine ---

TEST_CASE("Table::detectBoundariesFromLine splits on 2+ spaces", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	auto boundaries = accessor.detectBoundariesFromLine("Col1  Col2  Col3");

	REQUIRE(boundaries.size() == 3);
	REQUIRE(boundaries[0] == std::make_pair<std::size_t, std::size_t>(0, 4));
	REQUIRE(boundaries[1] == std::make_pair<std::size_t, std::size_t>(6, 10));
	REQUIRE(boundaries[2] == std::make_pair<std::size_t, std::size_t>(12, 16));
}

TEST_CASE("Table::detectBoundariesFromLine single space is not a gap", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	auto boundaries = accessor.detectBoundariesFromLine("hello world");

	REQUIRE(boundaries.size() == 1);
	REQUIRE(boundaries[0] == std::make_pair<std::size_t, std::size_t>(0, 11));
}

TEST_CASE("Table::detectBoundariesFromLine empty string", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	REQUIRE(accessor.detectBoundariesFromLine("").empty());
}

TEST_CASE("Table::detectBoundariesFromLine leading spaces", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	auto boundaries = accessor.detectBoundariesFromLine("  abc  def");

	REQUIRE(boundaries.size() == 2);
	REQUIRE(boundaries[0] == std::make_pair<std::size_t, std::size_t>(2, 5));
	REQUIRE(boundaries[1] == std::make_pair<std::size_t, std::size_t>(7, 10));
}

TEST_CASE("Table::detectBoundariesFromLine trailing spaces", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	auto boundaries = accessor.detectBoundariesFromLine("abc  def  ");

	REQUIRE(boundaries.size() == 2);
	REQUIRE(boundaries[0] == std::make_pair<std::size_t, std::size_t>(0, 3));
	REQUIRE(boundaries[1] == std::make_pair<std::size_t, std::size_t>(5, 8));
}

TEST_CASE("Table::detectBoundariesFromLine three-space gap", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	auto boundaries = accessor.detectBoundariesFromLine("abc   def");

	REQUIRE(boundaries.size() == 2);
	REQUIRE(boundaries[0].second == 3);
	REQUIRE(boundaries[1].first == 6);
}

// --- extractCellsByBoundaries ---

TEST_CASE("Table::extractCellsByBoundaries basic extraction", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	HTMLWriter::TableAccessor::Boundaries boundaries = {{0, 8}, {9, 12}};

	auto cells = accessor.extractCellsByBoundaries("Alpha    30 ", boundaries);
	REQUIRE(cells.size() == 2);
	REQUIRE(cells[0] == "Alpha");
	REQUIRE(cells[1] == "30");
}

TEST_CASE("Table::extractCellsByBoundaries empty when line is short", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	HTMLWriter::TableAccessor::Boundaries boundaries = {{0, 5}, {10, 15}};

	auto cells = accessor.extractCellsByBoundaries("ABC", boundaries);
	REQUIRE(cells.size() == 2);
	REQUIRE(cells[0] == "ABC");
	REQUIRE(cells[1] == "");
}

TEST_CASE("Table::extractCellsByBoundaries trims whitespace", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	HTMLWriter::TableAccessor::Boundaries boundaries = {{0, 10}, {10, 20}};

	auto cells = accessor.extractCellsByBoundaries("  hello     world   ", boundaries);
	REQUIRE(cells.size() == 2);
	REQUIRE(cells[0] == "hello");
	REQUIRE(cells[1] == "world");
}

TEST_CASE("Table::extractCellsByBoundaries three columns", "[table]") {
	HTMLWriter::Table table({});
	HTMLWriter::TableAccessor accessor{table};
	HTMLWriter::TableAccessor::Boundaries boundaries = {{0, 6}, {6, 12}, {12, 18}};

	auto cells = accessor.extractCellsByBoundaries("Alpha Bob   Carol ", boundaries);
	REQUIRE(cells.size() == 3);
	REQUIRE(cells[0] == "Alpha");
	REQUIRE(cells[1] == "Bob");
	REQUIRE(cells[2] == "Carol");
}

// --- parse: empty and invalid inputs ---

TEST_CASE("Table::parse empty lines produce invalid table", "[table]") {
	HTMLWriter::Table table({});
	table.parse();
	REQUIRE_FALSE(table.valid());
}

TEST_CASE("Table::parse no demarcation and too few lines is invalid", "[table]") {
	HTMLWriter::Table table({"just one line", "and another"});
	table.parse();
	REQUIRE_FALSE(table.valid());
}

TEST_CASE("Table::parse single line is invalid", "[table]") {
	HTMLWriter::Table table({"just one line"});
	table.parse();
	REQUIRE_FALSE(table.valid());
}

// --- parse: two-column key-value table ---

TEST_CASE("Table::parse two-column key-value table", "[table]") {
	HTMLWriter::Table table({
	    "Label     Value",
	    "-----     -----",
	    "Alpha     First",
	    "Bravo     Slow",
	    "Charlie   Third",
	    "Delta     Bane",
	    "Echo      Fifth",
	    "Foxtrot   Sixth",
	    "Golf      Burn",
	    " ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE_FALSE(accessor.isHeaderless());
	REQUIRE_FALSE(accessor.isPipeDelimited());
	REQUIRE(accessor.demarcationLine() == 1);
	REQUIRE(table.startLine() == 0);

	REQUIRE(accessor.rows().size() == 8);
	REQUIRE(accessor.rows()[0] == std::vector<std::string>{"Label", "Value"});
	REQUIRE(accessor.rows()[1] == std::vector<std::string>{"Alpha", "First"});
	REQUIRE(accessor.rows()[7] == std::vector<std::string>{"Golf", "Burn"});
}

// --- parse: two-column table with blank line after demarcation ---

TEST_CASE("Table::parse blank line after demarcation is skipped", "[table]") {
	HTMLWriter::Table table({
	    "COMMAND         RESULT",
	    "-----           ------",
	    " ",
	    "fxalpha         Dusty canopy ",
	    "zbravo          Foggy lenses ",
	    "qxporter        Short fable  ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 4);
	REQUIRE(accessor.rows()[0] == std::vector<std::string>{"COMMAND", "RESULT"});
	REQUIRE(accessor.rows()[1][0] == "fxalpha");
	REQUIRE(accessor.rows()[1][1] == "Dusty canopy");
	REQUIRE(accessor.rows()[3][0] == "qxporter");
	REQUIRE(accessor.rows()[3][1] == "Short fable");
}

// --- parse: wide 7-column table ---

TEST_CASE("Table::parse wide seven-column table", "[table]") {
	HTMLWriter::Table table({
	    "MODEL       SEATS    CARGO    BURN         RANGE    CREW             COST",
	    "-----       -----    -----    ----         -----    ----             ----",
	    " ",
	    "AB-320C     189        18     15,345       5,840    3/4              14.175",
	    "CD-720      165        16     11,000       6,835    3/3              14.025",
	    "EF-200      189        18     13,000       4,700    3/3              26",
	    "GH-400      168        16      3,050       3,870    2/3              41",
	    "IJ-800      184        18      3,050       5,420    2/3              48",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 6);
	REQUIRE(accessor.rows()[0].size() == 7);

	REQUIRE(accessor.rows()[0][0] == "MODEL");
	REQUIRE(accessor.rows()[0][3] == "BURN");
	REQUIRE(accessor.rows()[0][6] == "COST");

	REQUIRE(accessor.rows()[1][0] == "AB-320C");
	REQUIRE(accessor.rows()[1][1] == "189");
	REQUIRE(accessor.rows()[1][3] == "15,345");
	REQUIRE(accessor.rows()[1][6] == "14.175");

	REQUIRE(accessor.rows()[5][0] == "IJ-800");
}

// --- parse: wide table with continuation line ---

TEST_CASE("Table::parse wide table with continuation line", "[table]") {
	HTMLWriter::Table table({
	    "MODEL        SEATS    CARGO    BURN       RANGE    CREW             COST",
	    "-----        -----    -----    ----       -----    ----             ----",
	    " ",
	    "AB-1011      300        30      9,100     7,000    3/4              30 ",
	    " Variant 500",
	    "CDE Galaxy   345        34     14,000     6,000    3/5              25",
	    "FG Super 70  259        25     14,000    12,600    3/5              80",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() >= 4);
	// The continuation " Variant 500" should merge into the AB-1011 row
	REQUIRE(accessor.rows()[1][0].find("Variant") != std::string::npos);
}

// --- parse: misaligned header/data columns ---
// Pattern: header and demarcation have a large gap, data values shift within columns

TEST_CASE("Table::parse misaligned header and data columns", "[table]") {
	HTMLWriter::Table table({
	    "Lines read                             Tally",
	    "---------------------------         ---------",
	    "Great after party                     2 boxes",
	    "Flying saucer attack                  5 boxes",
	    "Unruly Viking maniac                 10 boxes",
	    "Best hair ever                       20 boxes",
	    "Star ship crew                       30 boxes",
	    "1 0 0                               100 boxes",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 7);

	REQUIRE(accessor.rows()[0][0] == "Lines read");
	REQUIRE(accessor.rows()[0][1] == "Tally");

	REQUIRE(accessor.rows()[1][0] == "Great after party");
	REQUIRE(accessor.rows()[1][1] == "2 boxes");
	REQUIRE(accessor.rows()[6][0] == "1 0 0");
	REQUIRE(accessor.rows()[6][1] == "100 boxes");
}

// --- parse: three-column table with part numbers ---
// Pattern: grouped sections separated by blank lines

TEST_CASE("Table::parse three-column table with grouped data rows", "[table]") {
	HTMLWriter::Table table({
	    "   part name        pins   maker      ",
	    "-----------------   ----   -----      ",
	    "XA77 3956AB TE9RJ     6    Petrov   ",
	    "XA77 3956AB ET9RJ     6    Pavlov",
	    "XA77 3966AB TE9RJ     6    Petrov ",
	    "XA77 3956AB LT9RJ     6    Polaris",
	    " ",
	    "XA77 8959CD  HI8J     9    Kohler  ",
	    "XA77 3959AB ET9RJ     9    Pavlov",
	    "XA77 3959AB TE9RJ     9    Petrov",
	    "XA77 3959CD  HI8J     9    Kohler ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() >= 9);

	REQUIRE(accessor.rows()[0][0] == "part name");
	REQUIRE(accessor.rows()[0][1] == "pins");
	REQUIRE(accessor.rows()[0][2] == "maker");

	REQUIRE(accessor.rows()[1][0] == "XA77 3956AB TE9RJ");
	REQUIRE(accessor.rows()[1][1] == "6");
	REQUIRE(accessor.rows()[1][2] == "Petrov");
}

// --- parse: three-column moves table ---
// Pattern: leading spaces in data, short notation values

TEST_CASE("Table::parse three-column moves table", "[table]") {
	HTMLWriter::Table table({
	    "Your move   Opp move    comment on your move",
	    "---------   --------    ---------------------",
	    "  F2-E3       D6-D5     move piece to center",
	    "  C2-D3       A6-B5     move piece to center",
	    "  B2-C3       A7-A6     move piece to center",
	    "  E3-E4       E6-E5     move piece to attack",
	    "  E4-E5 x     D5-E5 x   trade pieces",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 6);

	REQUIRE(accessor.rows()[0][0] == "Your move");
	REQUIRE(accessor.rows()[0][1] == "Opp move");
	REQUIRE(accessor.rows()[0][2] == "comment on your move");

	REQUIRE(accessor.rows()[1][0] == "F2-E3");
	REQUIRE(accessor.rows()[1][1] == "D6-D5");
	REQUIRE(accessor.rows()[1][2] == "move piece to center");
}

// --- parse: wide table with long wrapping cells ---
// Pattern: three columns where the last column wraps across multiple lines

TEST_CASE("Table::parse wide table with long wrapping cells", "[table]") {
	HTMLWriter::Table table({
	    "Name                   Type                 Effect",
	    "------------------------------------------------------------------------------------------------------------",
	    "Upholstered Bench      Tangible             Apply this to a surface that is not treated to get them to stick for",
	    "                                            roughly 45 minutes.",
	    " ",
	    "Brass Funnel           Decorative           When you fold napkins, there will be up to 2 fewer creased edges,",
	    "                                            one less awkward pleat shape, and 3 more clean sections.",
	    " ",
	    "Upholstered Bench 2   Tangible             Lasts twice as long as Upholstered Bench.",
	    " ",
	    "4 manila folders       N/A                  N/A",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());

	// "Upholstered Bench" + continuation should merge
	REQUIRE(accessor.rows()[1][0] == "Upholstered Bench");
	REQUIRE(accessor.rows()[1][1] == "Tangible");
	REQUIRE(accessor.rows()[1][2].find("45 minutes") != std::string::npos);
}

// --- parse: table with preceding non-empty text invalidates ---

TEST_CASE("Table::parse preceding non-empty text invalidates", "[table]") {
	HTMLWriter::Table table({
	    "Some intro text",
	    "",
	    "Name    Age",
	    "-----------",
	    "Alpha   30",
	});
	table.parse();

	REQUIRE_FALSE(table.valid());
}

// --- parse: blank lines before header set hasPrecedingText ---

TEST_CASE("Table::parse blank lines before header set hasPrecedingText", "[table]") {
	HTMLWriter::Table table({
	    "",
	    "Label    Value",
	    "-----------",
	    "Alpha    30",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(table.hasPrecedingText());
	REQUIRE(table.startLine() == 1);
	REQUIRE(accessor.rows().size() == 2);
	REQUIRE(accessor.rows()[0] == std::vector<std::string>{"Label", "Value"});
	REQUIRE(accessor.rows()[1] == std::vector<std::string>{"Alpha", "30"});
}

// --- parse: subsequent table detection ---

TEST_CASE("Table::parse stops before subsequent table header", "[table]") {
	HTMLWriter::Table table({
	    "Name    Age",
	    "-----------",
	    "Alpha   30",
	    "",
	    "City    Pop",
	    "-----------",
	    "Metro   8M",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(table.endLine() == 4);
	REQUIRE(accessor.rows().size() == 2);
	REQUIRE(accessor.rows()[0] == std::vector<std::string>{"Name", "Age"});
	REQUIRE(accessor.rows()[1] == std::vector<std::string>{"Alpha", "30"});
}

// --- parse: multi-row header ---

TEST_CASE("Table::parse multi-row header", "[table]") {
	HTMLWriter::Table table({
	    "           Year",
	    "Name    2023",
	    "-----------",
	    "Alpha   30",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 3);
	REQUIRE(table.startLine() == 0);
}

// --- parse: pipe-delimited grid table ---
// Pattern: numeric grid with pipe separators and symbolic values

TEST_CASE("Table::parse pipe-delimited grid table", "[table]") {
	HTMLWriter::Table table({
	    "  |01|02|03|04|05|06|07|08|09|10|11|12|13|14|15|16|17",
	    "-----------------------------------------------------",
	    "01|  |  |  |  |  |  |  |  |  |  |  |  |--|XX|  |  |--",
	    "02|  |--|--|++|  |++|  |  |  |  |  |++|--|  |--|  |++",
	    "03|  |++|--|--|  |  |  |  |++|  |  |  |++|  |--|  |",
	    "04|  |--|++|--|  |  |  |--|++|--|  |--|++|  |--|  |--",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isPipeDelimited());
	REQUIRE(accessor.rows().size() == 5);

	// Header row: empty cell before first pipe, then 01 through 17
	REQUIRE(accessor.rows()[0].size() == 18);
	REQUIRE(accessor.rows()[0][1] == "01");
	REQUIRE(accessor.rows()[0][17] == "17");

	REQUIRE(accessor.rows()[1][0] == "01");
	REQUIRE(accessor.rows()[1][13] == "--");
	REQUIRE(accessor.rows()[1][14] == "XX");

	REQUIRE(accessor.rows()[2][0] == "02");
	REQUIRE(accessor.rows()[2][2] == "--");
	REQUIRE(accessor.rows()[2][4] == "++");
}

TEST_CASE("Table::parse pipe-delimited pads short rows", "[table]") {
	HTMLWriter::Table table({
	    "|A|B|C|D",
	    "--------",
	    "|1|2",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isPipeDelimited());
	REQUIRE(accessor.rows().size() == 2);
	REQUIRE(accessor.rows()[1].size() == accessor.rows()[0].size());

	REQUIRE(accessor.rows()[0][1] == "A");
	REQUIRE(accessor.rows()[0][4] == "D");
	REQUIRE(accessor.rows()[1][1] == "1");
	REQUIRE(accessor.rows()[1][2] == "2");
	REQUIRE(accessor.rows()[1][3] == "");
	REQUIRE(accessor.rows()[1][4] == "");
}

TEST_CASE("Table::parse pipe-delimited skips empty data lines", "[table]") {
	HTMLWriter::Table table({
	    "|A|B|C",
	    "------",
	    "|1|2|3",
	    "",
	    "|4|5|6",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isPipeDelimited());
	REQUIRE(accessor.rows().size() == 3);
}

TEST_CASE("Table::parse pipe requires at least 3 pipes in header", "[table]") {
	HTMLWriter::Table table({
	    "A|B",
	    "---",
	    "1|2",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE_FALSE(accessor.isPipeDelimited());
}

// --- parse: char grid ---
// Pattern: single-character per column like "1 2 3 4 5"

TEST_CASE("Table::parse detects single-character grid", "[table]") {
	HTMLWriter::Table table({
	    "1 2 3 4 5",
	    "---------",
	    "A B C D E",
	    "F G H I J",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isCharGrid());
	REQUIRE(accessor.rows().size() == 3);

	REQUIRE(accessor.rows()[0] == std::vector<std::string>{"1", "2", "3", "4", "5"});
	REQUIRE(accessor.rows()[1] == std::vector<std::string>{"A", "B", "C", "D", "E"});
	REQUIRE(accessor.rows()[2] == std::vector<std::string>{"F", "G", "H", "I", "J"});
}

// --- parse: headerless tables ---

TEST_CASE("Table::parse headerless two-column table", "[table]") {
	HTMLWriter::Table table({
	    "Alpha    100",
	    "Bravo    200",
	    "Charlie  300",
	    "Delta    400",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isHeaderless());
	REQUIRE(table.startLine() == 0);
	REQUIRE(accessor.rows().size() == 4);

	REQUIRE(accessor.rows()[0][0] == "Alpha");
	REQUIRE(accessor.rows()[0][1] == "100");
	REQUIRE(accessor.rows()[1][0] == "Bravo");
	REQUIRE(accessor.rows()[1][1] == "200");
	REQUIRE(accessor.rows()[2][0] == "Charlie");
	REQUIRE(accessor.rows()[2][1] == "300");
	REQUIRE(accessor.rows()[3][0] == "Delta");
	REQUIRE(accessor.rows()[3][1] == "400");
}

// Pattern: wide first column and narrower second column, no header row

TEST_CASE("Table::parse headerless two-column wide-name table", "[table]") {
	HTMLWriter::Table table({
	    "",
	    "Sandwich of the Century          Replaced By",
	    "Plywood Shelf                    Thumbtack",
	    "Folded Pamphlet                  Cellophane",
	    "Rusty Bracket                    Turpentine",
	    "Wobbly Umbrella                  Clipboard",
	    "Painted Thimble                  Extension",
	    "Clipboard of Puzzles             Stepladder",
	    "Frayed Lacing                    Cardstock",
	    "Quilted Potholder                Marshmallow",
	    "Glazed Teacup                    Porcelain",
	    "Folded Bookmark                  Spiral Binding",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.isHeaderless());
	REQUIRE(accessor.rows().size() == 11);
	REQUIRE(accessor.rows()[0][0] == "Sandwich of the Century");
	REQUIRE(accessor.rows()[0][1] == "Replaced By");
	REQUIRE(accessor.rows()[1][0] == "Plywood Shelf");
	REQUIRE(accessor.rows()[1][1] == "Thumbtack");
}

TEST_CASE("Table::parse headerless requires at least 3 data lines", "[table]") {
	HTMLWriter::Table table({
	    "Alpha    100",
	    "Bravo    200",
	});
	table.parse();

	REQUIRE_FALSE(table.valid());
}

TEST_CASE("Table::parse headerless requires consensus on column count", "[table]") {
	HTMLWriter::Table table({
	    "a  b  c",
	    "d  e",
	    "f",
	    "g  h  i  j",
	});
	table.parse();

	REQUIRE_FALSE(table.valid());
}

// --- parse: data wider than header alignment ---
// Pattern: continuous demarcation, 5 columns, some data values wider than
// their header column causing column boundary adjustment

TEST_CASE("Table::parse five-column table with data wider than header", "[table]") {
	HTMLWriter::Table table({
	    "Ingredient         1st effect           2nd effect           3rd effect            4th effect",
	    "---------------------------------------------------------------------------------------------------",
	    "Tuba Song          Foggy Hamster        Crisp Harmonica      Plaid Fuzzy Envelope  Tangy Kazoo",
	    "Perforated Napkins Wrinkly Thimble      Peppy Thimble        Rowdy Thimble         Dusty Thimble",
	    "Upholstered Pillow   Dazed              Brisk Origami        Plaid Kazoo           Canopy",
	    "Dusty Planter      Plaid Kazoo          Tangy Kazoo          Dusty Thimble         Crisp Harmonica",
	    "Lumpy Crystal      Plaid Kazoo          Dusty Hamster        Rowdy Hamster         Dusty Plywood",
	    "Squat Timbre       Peppy Thimble        Rowdy Blanket        Dusty Harmonica       Fluffy Guitar",
	    "Lumpy Planter      Plaid Kazoo          Brisk Origami        Brisk Origami         Fluffy Guitar",
	    "Shiny Teacup       Dusty Clipboard      Crisp Trampoline     Crisp Max. Thimble    Tangy Zipper",
	    "Damp Slate         Plaid Kazoo          Brisk Origami        Lumpy Pendulum        Peppy Hamster",
	    "Worn Brick         Foggy Hamster        Rowdy Blanket        Plaid Fuzzy Envelope  Tangy Kazoo",
	    "Thin Bloom Cactus  Peppy Trampoline     Snowy Umbrella       Dusty Pencil          Dusty Thimble",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() == 12);
	REQUIRE(accessor.rows()[0].size() == 5);

	REQUIRE(accessor.rows()[0][0] == "Ingredient");
	REQUIRE(accessor.rows()[0][4] == "4th effect");

	// "Upholstered Pillow" is wider than header column — cell extraction should adjust
	REQUIRE(accessor.rows()[3][0] == "Upholstered Pillow");
	REQUIRE(accessor.rows()[3][1] == "Dazed");
	REQUIRE(accessor.rows()[3][4] == "Canopy");

	// Regular-width rows should still parse correctly
	REQUIRE(accessor.rows()[1][0] == "Tuba Song");
	REQUIRE(accessor.rows()[1][1] == "Foggy Hamster");
	REQUIRE(accessor.rows()[1][4] == "Tangy Kazoo");
}

// --- parse: three-column schedule with empty first-column continuation ---
// Pattern: grouped rows where blank first column indicates continuation of
// previous group, separated by blank lines between groups

TEST_CASE("Table::parse three-column schedule with grouped rows", "[table]") {
	HTMLWriter::Table table({
	    "Shelf label       Moved to          Day(s) available",
	    "----------------------------------------------------",
	    "Kettledrum        Trampoline        Mo/We/Sa",
	    "                  Tablecloth        Tu/Fr",
	    "  ",
	    "Thumbtacks        Kettledrum        Su",
	    "                  Bookmarks         Mo",
	    "                  Cardstock         Tu",
	    "                  Tablecloth        We",
	    "                  Trampoline        Th",
	    "                  Clipboard         Fr",
	    "  ",
	    "Trampoline        Tablecloth        Su",
	    "                  Bookmarks         Mo",
	    "                  Washboard         Tu",
	    "                  Cardstock         We",
	    "                  Clipboard         Th",
	    "                  Thumbtacks        Fr",
	    "                  Kettledrum        Sa",
	    "  ",
	    "Tablecloth        Kettledrum        Su/Tu/Th/Fr/Sa",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows()[0][0] == "Shelf label");
	REQUIRE(accessor.rows()[0][1] == "Moved to");
	REQUIRE(accessor.rows()[0][2] == "Day(s) available");

	// First data row
	REQUIRE(accessor.rows()[1][0] == "Kettledrum");
	REQUIRE(accessor.rows()[1][1] == "Trampoline");
	REQUIRE(accessor.rows()[1][2] == "Mo/We/Sa");
}

// --- parse: three-column table with multi-line item names and metadata ---
// Pattern: item names wrap across lines with indented metadata like "<27 Rating>"

TEST_CASE("Table::parse three-column table with multi-line continuations", "[table]") {
	HTMLWriter::Table table({
	    "Gadget                   Markup    Warehouse",
	    "--------------------------------------------------------------------------------------------",
	    "Bent Towel (checkered)      +3     Plywood shelf, crinkled behind doorframe",
	    "  <27 Tensile>",
	    "Crumpled Foil (lavender)    +1     Central Pantry Closet, Old Margery",
	    "  <20 Tensile>",
	    "Stepladder's Grip (hard     +1     Archive Cupboard Annex, quartermaster's closet",
	    "  maple) <26 Tensile>",
	    "The Kettle's Spout          +2     Southern Hallway, Marvin's closet",
	    "  (varnished) <30 Torsion>",
	    " ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() >= 5);

	REQUIRE(accessor.rows()[0][0] == "Gadget");
	REQUIRE(accessor.rows()[0][1] == "Markup");
	REQUIRE(accessor.rows()[0][2] == "Warehouse");

	// First item should have name with continuation merged
	REQUIRE(accessor.rows()[1][0].find("Bent Towel") != std::string::npos);
	REQUIRE(accessor.rows()[1][0].find("27 Tensile") != std::string::npos);
	REQUIRE(accessor.rows()[1][1] == "+3");
}

// --- parse: three-column table with deeply wrapped continuations ---
// Pattern: item name, material, AND stat requirements each get their
// own continuation line

TEST_CASE("Table::parse three-column table with three-line continuations", "[table]") {
	HTMLWriter::Table table({
	    "Gadget                 Markup    Warehouse",
	    "--------------------------------------------------------------------------------------------------------------",
	    "Hinge of the Cupboard     +2     Central Pantry Closet, sturdier shelf bracket (Part 5)",
	    "  (hard maple)",
	    "  <32 Tensile>",
	    "The Dull Sponge            +2     Garden Shed, Leonthar's closet after finishing three of the five sorting task",
	    "  (dampcloth)                       sets (counting the Bin of Twisted Ribbons chores as a separate set)",
	    "  <30 Torsion>",
	    "Grand Funnel (stainless)  +2     Found on final part of Hallway's \"Sock Drawer\" sorting task (Part 5)",
	    "  <34 Tensile>",
	    " ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() >= 4);

	REQUIRE(accessor.rows()[0][0] == "Gadget");
	REQUIRE(accessor.rows()[0][1] == "Markup");
	REQUIRE(accessor.rows()[0][2] == "Warehouse");

	// "Hinge of the Cupboard" should have material and stat requirement merged in
	REQUIRE(accessor.rows()[1][0].find("Hinge of the Cupboard") != std::string::npos);
	REQUIRE(accessor.rows()[1][0].find("hard maple") != std::string::npos);
	REQUIRE(accessor.rows()[1][0].find("32 Tensile") != std::string::npos);
	REQUIRE(accessor.rows()[1][1] == "+2");
}

// --- parse: table with star marker and trailing prose ---
// Pattern: "*" marks an initial item and trailing text after a blank line
// explains the marker

TEST_CASE("Table::parse table with special marker values and trailing note", "[table]") {
	HTMLWriter::Table table({
	    "Level   Step",
	    "-----   ----------",
	    " *      Jumble",
	    " 10     Cartwheel",
	    " 20     Half Twirl",
	    " 30     Pivot Turn",
	    " 40     Backwards",
	    " 50     Triple Stitch (Left Only)",
	    " ",
	    "* = Known from the start.",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());
	REQUIRE(accessor.rows().size() >= 7);

	REQUIRE(accessor.rows()[0][0] == "Level");
	REQUIRE(accessor.rows()[0][1] == "Step");

	REQUIRE(accessor.rows()[1][0] == "*");
	REQUIRE(accessor.rows()[1][1] == "Jumble");

	REQUIRE(accessor.rows()[2][0] == "10");
	REQUIRE(accessor.rows()[2][1] == "Cartwheel");

	REQUIRE(accessor.rows()[6][0] == "50");
	REQUIRE(accessor.rows()[6][1] == "Triple Stitch (Left Only)");
}

// --- parse: two-column table with single-word title and multi-line data ---
// Pattern: header is a single title spanning the width, data rows have two
// columns with wrapped continuations. Data-driven column boundary refinement
// splits the title header into two columns.

TEST_CASE("Table::parse two-column table with title header refined by data", "[table]") {
	HTMLWriter::Table table({
	    "Bookmarks and Annotations",
	    "-------------------------",
	    "glued to cardboard     (100% Matte Laminate)",
	    "xr_strbooster          (Thicken Binding 25 pts, Deboss 50%, Matte Dust Jackets",
	    "                        100%, Matte Endpaper 50%, Matte Foxing 100%, Matte",
	    "                        Laminate 100%)",
	    "xr_spdbooster          (Repair Crease and Dogears 5 pts each, Deboss 50%,",
	    "                        Matte Dust Jackets 100%, Matte Endpaper 50%, Matte",
	    "                        Foxing 100%, Matte Laminate 100%)",
	    " ",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();

	REQUIRE(table.valid());

	// Header gets split by data column boundary refinement into 2 columns
	REQUIRE(accessor.rows()[0].size() == 2);

	// First data row
	REQUIRE(accessor.rows()[1][0] == "glued to cardboard");
	REQUIRE(accessor.rows()[1][1] == "(100% Matte Laminate)");

	// Second data row has continuation lines merged
	REQUIRE(accessor.rows()[2][0] == "xr_strbooster");
	REQUIRE(accessor.rows()[2][1].find("Thicken Binding 25 pts") != std::string::npos);
	REQUIRE(accessor.rows()[2][1].find("Laminate 100%") != std::string::npos);
}

// --- serialize: DOM structure verification ---

static pugi::xml_node findChild(pugi::xml_node parent, char const* name) {
	return parent.child(name);
}

static int countChildren(pugi::xml_node parent, char const* name) {
	int count = 0;
	for (auto child = parent.child(name); child; child = child.next_sibling(name)) {
		++count;
	}
	return count;
}

static std::vector<std::string> getRowCells(pugi::xml_node row, char const* cellTag) {
	std::vector<std::string> cells;
	for (auto cell = row.child(cellTag); cell; cell = cell.next_sibling(cellTag)) {
		cells.push_back(cell.child_value());
	}
	return cells;
}

TEST_CASE("Table::serialize demarcation table structure", "[table]") {
	HTMLWriter::Table table({
	    "Name    Age",
	    "-----------",
	    "Alpha   30",
	    "Bravo   25",
	});
	table.parse();
	REQUIRE(table.valid());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	REQUIRE(std::string(container.attribute("class").value()) == "table-container");

	auto tableNode = findChild(container, "table");
	REQUIRE(tableNode);
	REQUIRE(
	    std::string(tableNode.attribute("class").value()) == "option option-html hidden");

	// <thead>
	auto thead = findChild(tableNode, "thead");
	REQUIRE(thead);
	REQUIRE(countChildren(thead, "tr") == 1);

	auto headerRow = findChild(thead, "tr");
	REQUIRE(getRowCells(headerRow, "th") == std::vector<std::string>{"Name", "Age"});

	// <tbody>
	auto tbody = findChild(tableNode, "tbody");
	REQUIRE(tbody);
	REQUIRE(countChildren(tbody, "tr") == 2);

	auto dataRows = tbody.children("tr");
	auto rowIterator = dataRows.begin();
	REQUIRE(getRowCells(*rowIterator, "td") == std::vector<std::string>{"Alpha", "30"});
	++rowIterator;
	REQUIRE(getRowCells(*rowIterator, "td") == std::vector<std::string>{"Bravo", "25"});

	// Fallback div
	auto fallbackDiv = findChild(container, "div");
	REQUIRE(fallbackDiv);
	REQUIRE(std::string(fallbackDiv.attribute("class").value())
	        == "option option-text monospace overflow");
	REQUIRE(countChildren(fallbackDiv, "br") == 3);
}

TEST_CASE("Table::serialize headerless table omits thead", "[table]") {
	HTMLWriter::Table table({
	    "Alpha    100",
	    "Bravo    200",
	    "Charlie  300",
	    "Delta    400",
	});
	table.parse();
	REQUIRE(table.valid());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	auto tableNode = findChild(container, "table");
	REQUIRE(tableNode);
	REQUIRE_FALSE(findChild(tableNode, "thead"));

	auto tbody = findChild(tableNode, "tbody");
	REQUIRE(tbody);
	REQUIRE(countChildren(tbody, "tr") == 4);

	auto firstRow = findChild(tbody, "tr");
	auto cells = getRowCells(firstRow, "td");
	REQUIRE(cells[0] == "Alpha");
	REQUIRE(cells[1] == "100");
}

TEST_CASE("Table::serialize single-header-cell gets colspan", "[table]") {
	HTMLWriter::Table table({
	    "Title",
	    "Name    Age",
	    "-----------",
	    "Alpha   30",
	});
	table.parse();
	REQUIRE(table.valid());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	auto tableNode = findChild(container, "table");
	auto thead = findChild(tableNode, "thead");
	REQUIRE(thead);

	auto firstHeaderRow = findChild(thead, "tr");
	REQUIRE(firstHeaderRow);

	auto thNode = findChild(firstHeaderRow, "th");
	REQUIRE(thNode);
	REQUIRE(thNode.attribute("colspan"));
	REQUIRE(thNode.attribute("colspan").as_int() == 2);
	REQUIRE(std::string(thNode.child_value()) == "Title");
}

TEST_CASE("Table::serialize text fallback preserves line count and content", "[table]") {
	HTMLWriter::Table table({
	    "Name    Age",
	    "-----------",
	    "Alpha   30",
	    "Bravo   25",
	    "Charlie 22",
	});
	table.parse();
	REQUIRE(table.valid());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	auto fallbackDiv = findChild(container, "div");
	REQUIRE(fallbackDiv);

	// 5 lines = 4 <br> separators
	REQUIRE(countChildren(fallbackDiv, "br") == 4);

	// Collect all text content from fallback
	std::string textContent;
	for (auto child = fallbackDiv.first_child(); child; child = child.next_sibling()) {
		if (child.type() == pugi::node_pcdata) {
			textContent += child.value();
		}
	}
	REQUIRE(textContent.find("Name") != std::string::npos);
	REQUIRE(textContent.find("Alpha") != std::string::npos);
	REQUIRE(textContent.find("Charlie") != std::string::npos);
}

TEST_CASE("Table::serialize wide table preserves all columns", "[table]") {
	HTMLWriter::Table table({
	    "Loc#  Name         Cell       X          Y         Z",
	    "-----------------------------------------------------",
	    "  1   Base         -27,27  -113009.0  116675.2  17214.9",
	    "  2   Station      -27,25  -108378.3  103560.3  15177.0",
	});
	table.parse();
	REQUIRE(table.valid());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	auto tableNode = findChild(container, "table");
	auto thead = findChild(tableNode, "thead");
	auto headerRow = findChild(thead, "tr");
	auto headerCells = getRowCells(headerRow, "th");
	REQUIRE(headerCells.size() == 6);
	REQUIRE(headerCells[0] == "Loc#");
	REQUIRE(headerCells[5] == "Z");

	auto tbody = findChild(tableNode, "tbody");
	auto firstDataRow = findChild(tbody, "tr");
	auto dataCells = getRowCells(firstDataRow, "td");
	REQUIRE(dataCells.size() == 6);
	REQUIRE(dataCells[0] == "1");
	REQUIRE(dataCells[1] == "Base");
}

TEST_CASE("Table::serialize pipe-delimited table structure", "[table]") {
	HTMLWriter::Table table({
	    "|A|B|C",
	    "------",
	    "|1|2|3",
	    "|4|5|6",
	});
	HTMLWriter::TableAccessor accessor{table};
	table.parse();
	REQUIRE(table.valid());
	REQUIRE(accessor.isPipeDelimited());

	pugi::xml_document xmlDocument;
	auto container = xmlDocument.append_child("div");
	table.serialize(container);

	auto tableNode = findChild(container, "table");
	auto tbody = findChild(tableNode, "tbody");
	REQUIRE(tbody);
	REQUIRE(countChildren(tbody, "tr") == 2);

	auto firstDataRow = findChild(tbody, "tr");
	auto cells = getRowCells(firstDataRow, "td");

	bool hasOne = false;
	bool hasTwo = false;
	bool hasThree = false;
	for (auto const& cell : cells) {
		if (cell == "1") {
			hasOne = true;
		}
		if (cell == "2") {
			hasTwo = true;
		}
		if (cell == "3") {
			hasThree = true;
		}
	}
	REQUIRE(hasOne);
	REQUIRE(hasTwo);
	REQUIRE(hasThree);
}

} // namespace UHS
