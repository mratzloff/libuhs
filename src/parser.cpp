#include "uhs.h"
#include <climits>
#include <iostream>
#include <thread>

namespace UHS {

Parser::Parser(const ParserOptions options) : options_{options} {
	// TODO: Guard these by platform
	setenv("TZ", "", 1);
	tzset();
	// _putenv_s("TZ", ""); // MSVC
	// _tzset(); // MSVC
}

std::unique_ptr<Document> Parser::parse(std::ifstream& in) {
	auto pipe = std::make_unique<Pipe>(in);
	tokenizer_ = std::make_unique<Tokenizer>(*pipe);
	crc_ = std::make_unique<CRC>();
	crc_->upstream(*pipe);

	// Interleave disk reads with tokenization and CRC calculation
	std::thread thread{[&pipe] { pipe->read(); }};

	try {
		// Meanwhile, build out document by parsing emitted tokens in parallel
		document_ = Document::create(VersionType::Version88a);

		this->parse88a();

		if (!done_ && !options_.force88aMode) {
			// Set 88a header as first (hidden) child of 96a document
			document_->visibility(VisibilityType::None);

			auto document = Document::create(VersionType::Version96a);
			document_.swap(document);
			document_->appendChild(std::move(document));

			this->parse96a();
		}

		thread.join();
		return std::move(document_);
	} catch (const Error& err) {
		thread.join();
		throw;
	}
}

void Parser::reset() {
	tokenizer_.reset();
	crc_.reset();
	document_.reset();
	parents_.clear();
	deferredLinkChecks_.clear();
	dataHandlers_.clear();
	key_.clear();
	lineOffset_ = 0;
	isTitleSet_ = false;
	done_ = false;
}

//--------------------------------- UHS 88a ---------------------------------//

void Parser::parse88a() {
	// Signature
	this->expect(TokenType::Signature);

	// Title
	auto token = this->expect(TokenType::String);
	document_->title(token->value());

	// First hint line
	token = this->expect(TokenType::Line);
	int firstHintTextLine;
	try {
		firstHintTextLine = Strings::toInt(token->value());
		if (firstHintTextLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	firstHintTextLine += HeaderLength;

	// Last hint line
	token = this->expect(TokenType::Line);
	int lastHintTextLine;
	try {
		lastHintTextLine = Strings::toInt(token->value());
		if (lastHintTextLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	lastHintTextLine += HeaderLength;

	// Subject and hint elements
	NodeMap parents{{0, document_.get()}};
	this->parse88aElements(firstHintTextLine, parents);

	// Hint text nodes
	this->parse88aTextNodes(lastHintTextLine, parents);

	// Credits

	// Anything from this point on is part of the 88a credits
	// until EOF or the backwards compatibility token.
	token = this->next();

	auto tokenType = token->type();
	auto line = token->line();
	auto column = token->column();

	switch (tokenType) {
	case TokenType::CreditSep: // This is informal but common
		[[fallthrough]];
	case TokenType::String:
		this->parse88aCredits(std::move(token));
		return;
	case TokenType::HeaderSep:
		this->parseHeaderSep(std::move(token));
		return;
	case TokenType::FileEnd:
		done_ = true;
		return;
	default:
		throw ParseError::badToken(line, column, tokenType);
	}
}

// This (correctly) errors out on sq4g.uhs, which appears to be miscompiled
// from the original 88a precompile format. The official Windows reader
// doesn't support it, and as far as I can tell no one bothered to ever check
// it. It mostly works in the DOS reader, but it looks glitchy.
void Parser::parse88aElements(int firstHintTextLine, NodeMap& parents) {
	auto elementType = ElementType::Subject;

	for (;;) {
		auto token = this->expect(TokenType::String);
		std::string encodedTitle{token->value()};
		auto line = token->line();

		token = this->expect(TokenType::Line);
		int firstChildLine;
		try {
			firstChildLine = Strings::toInt(token->value());
			if (firstChildLine < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			throw ParseError::badValue(
			    token->line(), token->column(), ParseError::Uint, token->value());
		}
		firstChildLine += HeaderLength;

		if (firstChildLine == firstHintTextLine) {
			elementType = ElementType::Hint;
		}

		Node* parent = nullptr;
		for (auto i = line; !parent; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (!parent) {
			throw ParseError(
			    token->line(), token->column(), "could not find parent node");
		}

		auto element = Element::create(elementType, line);
		element->line(line);
		auto e = element.get();
		parent->appendChild(std::move(element));

		std::string title{codec_.decode88a(encodedTitle)};
		if (parent->nodeType() != NodeType::Document) {
			char finalChar = title[title.length() - 1];
			if (!this->isPunctuation(finalChar)) {
				title += '?';
			}
		}
		e->title(title);

		if (line < firstHintTextLine) {
			parents[firstChildLine] = e;
		}
		if (line + 2 == firstHintTextLine) {
			break; // Done parsing subjects
		}
	}
}

void Parser::parse88aTextNodes(int lastHintTextLine, NodeMap& parents) {
	auto line = 0;

	do {
		auto token = this->expect(TokenType::String);
		line = token->line();

		Node* parent = nullptr;
		for (auto i = line; !parent; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (!parent) {
			throw ParseError(
			    token->line(), token->column(), "could not find parent node");
		}
		if (parent->nodeType() != NodeType::Element) {
			throw ParseError(token->line(),
			    token->column(),
			    "unexpected parent type: %s",
			    parent->nodeTypeString());
		}

		auto& element = static_cast<Element&>(*parent);
		element.appendChild(codec_.decode88a(token->value()));
	} while (line < lastHintTextLine);
}

void Parser::parse88aCredits(std::unique_ptr<const Token> token) {
	std::string body;
	auto continuation = false;

	for (;;) {
		switch (token->type()) {
		case TokenType::FileEnd:
			if (!body.empty()) {
				document_->attr("notice", body);
			}
			done_ = true;
			return;
		case TokenType::HeaderSep:
			if (!body.empty()) {
				document_->attr("notice", body);
			}
			this->parseHeaderSep(std::move(token));
			return;
		case TokenType::CreditSep:
			break; // Ignore
		case TokenType::String:
			if (continuation) {
				body += ' ';
			}
			body += token->value();
			continuation = true;
			break;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}

		token = this->next();
	}
}

//--------------------------------- UHS 96a ---------------------------------//

void Parser::parseHeaderSep(std::unique_ptr<const Token> token) {
	if (options_.force88aMode) {
		done_ = true;
		return;
	}
	// 96a element line numbers don't include compatibility header
	lineOffset_ = token->line();
}

void Parser::parse96a() {
	parents_.add(*document_, 0, INT_MAX);

	// Parse elements
	for (;;) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::Length: {
			this->parseElement(std::move(token));
			break;
		}
		case TokenType::Data:
			this->parseData(std::move(token));
			break;
		case TokenType::CRC:
			this->checkCRC();
			break;
		case TokenType::FileEnd:
			done_ = true;
			return;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

	this->checkLinks();
}

// Elements are automatically appended to their parents
Element* Parser::parseElement(std::unique_ptr<const Token> token) {
	Element* e;

	// Length
	int length;
	try {
		length = Strings::toInt(token->value());
		if (length < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}

	// Ident
	token = this->expect(TokenType::Ident);
	const auto& ident = token->value();

	// Create element
	auto elementType = Element::elementType(ident);
	int line = this->offsetLine(token->line());
	auto element = Element::create(elementType, line);
	element->line(line);
	element->length(length);
	e = element.get();

	try {
		this->findParentAndAppend(std::move(element));
	} catch (const Error& err) {
		std::throw_with_nested(ParseError(token->line(),
		    token->column(),
		    "orphaned element: %s",
		    element->elementTypeString()));
	}

	// Title
	token = this->expect(TokenType::String);
	e->title(token->value());

	switch (elementType) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		this->parseCommentElement(e);
		break;
	case ElementType::Credit:
		this->parseCommentElement(e);
		break;
	case ElementType::Gifa:
		this->parseDataElement(e);
		break;
	case ElementType::Hint:
		this->parseHintElement(e);
		break;
	case ElementType::Hyperpng:
		this->parseHyperpngElement(e);
		break;
	case ElementType::Incentive:
		this->parseIncentiveElement(e);
		break;
	case ElementType::Info:
		this->parseInfoElement(e);
		break;
	case ElementType::Link:
		this->parseLinkElement(e);
		break;
	case ElementType::Nesthint:
		this->parseHintElement(e);
		break;
	case ElementType::Overlay:
		this->parseOverlayElement(e);
		break;
	case ElementType::Sound:
		this->parseDataElement(e);
		break;
	case ElementType::Subject:
		this->parseSubjectElement(e);
		break;
	case ElementType::Text:
		this->parseTextElement(e);
		break;
	case ElementType::Version:
		this->parseVersionElement(e);
		break;
	}

	return e;
}

void Parser::parseCommentElement(Element* const element) {
	auto length = element->length();
	std::string body;
	auto continuation = false;

	for (auto i = 3; i <= length; ++i) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::String:
			if (continuation) {
				body += ' ';
			}
			body += token->value();
			continuation = true;
			break;
		case TokenType::NestedTextSep:
			[[fallthrough]];
		case TokenType::NestedParagraphSep:
			body += "\n\n";
			continuation = false;
			break;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

	element->body(body);
}

void Parser::parseDataElement(Element* const element) {
	// Offset
	std::size_t offset;

	for (;;) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::DataOffset:
			int intOffset;
			try {
				intOffset = Strings::toInt(token->value());
				if (intOffset < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(
				    token->line(), token->column(), ParseError::Uint, token->value());
			}
			offset = intOffset;
			goto expectDataLength;
		case TokenType::TextFormat:
			continue; // Ignore
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

expectDataLength:
	// Length
	auto token = this->expect(TokenType::DataLength);
	int intLength;
	try {
		intLength = Strings::toInt(token->value());
		if (intLength < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	std::size_t length = intLength;

	// Data
	this->addDataCallback(offset, length, [=](std::string data) { element->body(data); });
}

// A hint element ending with a nested text separator ("-") should be an
// error, but bluforce.uhs has an instance of this. This actually screws
// up the official reader UI for that particular hint.
void Parser::parseHintElement(Element* const element) {
	std::string hintText;

	// Parse child elements
	auto length = element->length();
	auto continuation = false;

	for (auto i = 3; i <= length; ++i) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::String:
			if (continuation) {
				hintText += ' ';
			}
			if (element->elementType() == ElementType::Nesthint) {
				hintText += codec_.decode96a(token->value(), key_, false);
			} else {
				hintText += codec_.decode88a(token->value());
			}
			continuation = true;
			break;
		case TokenType::NestedTextSep:
			if (!hintText.empty()) {
				element->appendChild(hintText);
				hintText.clear();
			}
			continuation = false;
			break;
		case TokenType::NestedParagraphSep:
			hintText += "\n\n";
			continuation = false;
			break;
		case TokenType::NestedElementSep:
			if (i == length || element->elementType() != ElementType::Nesthint) {
				throw ParseError::badValue(
				    token->line(), token->column(), ParseError::String, token->value());
			}

			// Append current text node
			if (!hintText.empty()) {
				element->appendChild(hintText);
				hintText.clear();
			}
			continuation = false;

			// Length
			token = this->expect(TokenType::Length);

			{ // Child element
				const auto child = this->parseElement(std::move(token));
				i += child->length();
			}
			break;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

	// Append current text node
	if (!hintText.empty()) {
		element->appendChild(hintText);
	}
}

void Parser::parseHyperpngElement(Element* const element) {
	this->parseDataElement(element);

	// Parse child elements
	auto length = element->length();
	auto childLength = 0;

	for (auto i = 4; i < length; i += 1 + childLength) {
		// Interactive region top-left X coordinate
		auto token = this->expect(TokenType::CoordX);
		auto x1 = token->value();
		try {
			Strings::toInt(x1);
		} catch (const Error& err) {
			throw ParseError::badValue(
			    token->line(), token->column(), ParseError::Int, x1);
		}

		// Interactive region top-left Y coordinate
		token = this->expect(TokenType::CoordY);
		auto y1 = token->value();
		try {
			Strings::toInt(y1);
		} catch (const Error& err) {
			throw ParseError::badValue(
			    token->line(), token->column(), ParseError::Int, y1);
		}

		// Interactive region bottom-right X coordinate
		token = this->expect(TokenType::CoordX);
		auto x2 = token->value();
		try {
			Strings::toInt(x2);
		} catch (const Error& err) {
			throw ParseError::badValue(
			    token->line(), token->column(), ParseError::Int, x2);
		}

		// Interactive region bottom-right Y coordinate
		token = this->expect(TokenType::CoordY);
		auto y2 = token->value();
		try {
			Strings::toInt(y2);
		} catch (const Error& err) {
			throw ParseError::badValue(
			    token->line(), token->column(), ParseError::Int, y2);
		}

		// Child element (overlay or link)
		token = this->expect(TokenType::Length);
		const auto child = this->parseElement(std::move(token));
		childLength = child->length();

		// Backfill coordinates
		child->attr("region:top-left-x", x1);
		child->attr("region:top-left-y", y1);
		child->attr("region:bottom-right-x", x2);
		child->attr("region:bottom-right-y", y2);
	}
}

// A bad instruction ("12") is found at the beginning or end of the incentive
// list for four files, so we skip bad instructions of that form. Fourteen
// other files have lines pointing to nowhere, so we skip those, too.
void Parser::parseIncentiveElement(Element* const element) {
	std::string body;

	element->visibility(VisibilityType::None);

	// Read and decode visibility instructions
	auto length = element->length();
	auto continuation = false;

	for (auto i = 2; i < length; ++i) {
		auto token = this->expect(TokenType::String);
		if (continuation) {
			body += ' ';
		}
		body += codec_.decode96a(token->value(), key_, false);
		continuation = true;
	}
	element->body(body);

	if (body.empty()) {
		return; // No instructions to process
	}

	// Process instructions
	auto markers = Strings::split(body, " ");
	std::size_t markerLength;

	for (const auto& marker : markers) {
		markerLength = marker.length();

		// Split instruction (e.g., "3Z")
		if (markerLength < 2) {
			continue; // TODO: Warn
		}

		int targetLine;
		try {
			targetLine = Strings::toInt(marker.substr(0, markerLength - 1));
			if (targetLine < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			continue; // TODO: Warn
		}

		// Set visibility
		if (auto target = this->findTarget(targetLine)) {
			auto instruction = marker.substr(markerLength - 1, 1);

			if (instruction == Token::Registered) {
				target->visibility(VisibilityType::Registered);
			} else if (instruction == Token::Unregistered) {
				target->visibility(VisibilityType::Unregistered);
			}
		}
	}
}

void Parser::parseInfoElement(Element* const element) {
	std::string serialized;
	std::string key;
	std::string value;
	std::string date;
	std::tm tm;

	element->visibility(VisibilityType::None);

	// Key-value pairs followed by a notice
	auto length = element->length();

	for (auto i = 2; i < length; ++i) {
		auto token = this->expect(TokenType::String);
		serialized = token->value();

		if (serialized.substr(0, 1) == Token::NoticePrefix) {
			key = "notice";
			value = serialized.substr(1);
		} else {
			auto parts = Strings::split(serialized, Token::InfoKeyValueSep, 2);
			if (parts.size() != 2) {
				throw ParseError::badValue(
				    token->line(), token->column(), "key-value pair", token->value());
			}
			key = parts[0];
			value = parts[1];
		}

		if (key == "length") {
			int fileLength;
			try {
				fileLength = Strings::toInt(value);
				if (fileLength < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(
				    token->line(), token->column(), ParseError::Uint, value);
			}
			document_->attr("length", std::to_string(fileLength));
		} else if (key == "date") {
			try {
				this->parseDate(value, tm);
			} catch (const Error& err) {
				std::throw_with_nested(ParseError::badValue(
				    token->line(), token->column(), ParseError::Date, value));
			}
		} else if (key == "time") {
			try {
				this->parseTime(value, tm);
			} catch (const Error& err) {
				std::throw_with_nested(ParseError::badValue(
				    token->line(), token->column(), ParseError::Time, value));
			}
			char buffer[20];
			auto bufferLength = std::strftime(buffer, 20, "%Y-%m-%dT%H:%M:%S", &tm);
			document_->attr("timestamp", std::string(buffer, bufferLength));
		} else {
			if (auto attrValue = document_->attr(key)) {
				document_->attr(key, *attrValue + " " + value);
			} else {
				document_->attr(key, value);
			}
		}
	}
}

void Parser::parseLinkElement(Element* const element) {
	// Ref line
	auto token = this->expect(TokenType::Line);
	auto body = token->value();
	int targetLine;
	try {
		targetLine = Strings::toInt(body);
		if (targetLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, body);
	}
	element->body(body);

	this->deferLinkCheck(targetLine, token->line(), token->column());
}

void Parser::parseOverlayElement(Element* const element) {
	this->parseDataElement(element);

	// Image top-left X coordinate
	auto token = this->expect(TokenType::CoordX);
	auto x = token->value();
	try {
		Strings::toInt(x);
	} catch (const Error& err) {
		throw ParseError::badValue(token->line(), token->column(), ParseError::Int, x);
	}
	element->attr("image:x", x);

	// Image bottom-right Y coordinate
	token = this->expect(TokenType::CoordY);
	auto y = token->value();
	try {
		Strings::toInt(y);
	} catch (const Error& err) {
		throw ParseError::badValue(token->line(), token->column(), ParseError::Int, y);
	}
	element->attr("image:y", y);
}

void Parser::parseSubjectElement(Element* const element) {
	std::unique_ptr<const Token> token;

	if (!isTitleSet_) {
		document_->title(element->title());
		key_ = codec_.createKey(document_->title());
		isTitleSet_ = true;
	}

	const auto length = element->length();
	auto childLength = 0;

	for (auto i = 3; i <= length; i += childLength) {
		// Length
		auto token = this->expect(TokenType::Length);

		// Child element
		const auto child = this->parseElement(std::move(token));
		childLength = child->length();
	}
}

void Parser::parseTextElement(Element* const element) {
	// Format
	auto token = this->expect(TokenType::TextFormat);
	int format;
	try {
		format = Strings::toInt(token->value());
		if (format < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	if (format > 3) {
		throw ParseError(token->line(),
		    token->column(),
		    "text format byte must be between 0 and 3; found %d",
		    format);
	}
	const auto formatType = static_cast<TextFormatType>(format);

	if (formatType == TextFormatType::Monospace
	    || formatType == TextFormatType::MonospaceAlt) {
		element->attr("typeface", "monospace");
	} else {
		element->attr("typeface", "proportional");
	}

	// Offset
	token = this->expect(TokenType::DataOffset);
	int intOffset;
	try {
		intOffset = Strings::toInt(token->value());
		if (intOffset < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	std::size_t offset = intOffset;

	// Length
	token = this->expect(TokenType::DataLength);
	int intLength;
	try {
		intLength = Strings::toInt(token->value());
		if (intLength < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(
		    token->line(), token->column(), ParseError::Uint, token->value());
	}
	std::size_t length = intLength;

	// Data
	this->addDataCallback(offset, length, [=](std::string data) {
		auto lines = Strings::split(data, UHS::EOL);
		for (auto& line : lines) {
			line = codec_.decode96a(line, key_, true);
			if (line == Token::ParagraphSep) {
				line = "";
			}
		}
		auto value = Strings::rtrim(Strings::join(lines, "\n"), '\n');
		element->body(value);
	});
}

void Parser::parseVersionElement(Element* const element) {
	element->visibility(VisibilityType::None);
	this->parseCommentElement(element);

	auto version = element->title();
	if (version == "91a") {
		document_->version(VersionType::Version91a);
	} else if (version == "95a") {
		document_->version(VersionType::Version95a);
	}
}

std::unique_ptr<const Token> Parser::next() {
	auto token = tokenizer_->next();

	if (options_.debug) {
		std::cout << token->string() << std::endl;
	}
	return token;
}

std::unique_ptr<const Token> Parser::expect(TokenType expected) {
	auto token = this->next();
	auto tokenType = token->type();

	if (tokenType == TokenType::FileEnd && expected != TokenType::FileEnd) {
		throw ParseError(token->line(), token->column(), "unexpected end of file");
	} else if (tokenType != expected) {
		throw ParseError::badToken(
		    token->line(), token->column(), expected, token->type());
	}
	return token;
}

void Parser::findParentAndAppend(std::unique_ptr<Element> element) {
	assert(element);

	auto min = element->line();
	auto max = min + element->length();
	auto parent = parents_.find(min, max);
	parents_.add(*element, min, max);

	if (!parent) {
		throw Error("could not find parent element between lines %d and %d", min, max);
	}
	parent->appendChild(std::move(element));
}

void Parser::deferLinkCheck(int targetLine, const int line, const int column) {
	deferredLinkChecks_.emplace_back(targetLine, line, column);
}

void Parser::checkLinks() {
	for (const auto [targetLine, line, column] : deferredLinkChecks_) {
		if (const auto target = this->findTarget(targetLine); !target) {
			// TODO: Warning using ld.line and ld.column
		}
	}
}

// A link pointing to the child of a hyperpng element is a special case. Instead of
// referencing the descriptor line, it points to the region, even if it doesn't
// share a parent hyperpng. A link outside of a hyperpng pointing to a child of
// a hyperpng references the descriptor line, as usual.
Element* Parser::findTarget(const int line) {
	auto target = document_->find(line);
	if (!target) {
		target = document_->find(line + 1);
		if (!target) {
			return nullptr;
		}
		if (const auto parent = target->parent()) {
			const auto& parentElement = static_cast<Element&>(*parent);
			if (parentElement.elementType() != ElementType::Hyperpng) {
				return nullptr; // We found an element, but shouldn't have
			}
		}
	}
	return target;
}

void Parser::addDataCallback(std::size_t offset, std::size_t length, DataCallback func) {
	dataHandlers_.push_back(DataHandler(offset, length, func));
}

void Parser::parseData(std::unique_ptr<const Token> token) {
	std::size_t offset;

	for (const auto& handler : dataHandlers_) {
		offset = handler.offset - token->offset();
		handler.func(token->value().substr(offset, handler.length));
	}
}

void Parser::checkCRC() {
	document_->validChecksum(crc_->valid());
}

// Format: DD-Mon-YY
void Parser::parseDate(const std::string& date, std::tm& tm) const {
	auto parts = Strings::split(date, "-", 3);

	int year;
	try {
		year = Strings::toInt(parts[2]);
		if (year < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid year: %d", year);
	}
	if (year < 95) { // Info elements were introduced in 1995
		year += 100;
	}

	int month;
	if (parts[1] == "Jan") {
		month = 0;
	} else if (parts[1] == "Feb") {
		month = 1;
	} else if (parts[1] == "Mar") {
		month = 2;
	} else if (parts[1] == "Apr") {
		month = 3;
	} else if (parts[1] == "May") {
		month = 4;
	} else if (parts[1] == "Jun") {
		month = 5;
	} else if (parts[1] == "Jul") {
		month = 6;
	} else if (parts[1] == "Aug") {
		month = 7;
	} else if (parts[1] == "Sep") {
		month = 8;
	} else if (parts[1] == "Oct") {
		month = 9;
	} else if (parts[1] == "Nov") {
		month = 10;
	} else if (parts[1] == "Dec") {
		month = 11;
	} else {
		throw Error("invalid month: %d", parts[1]);
	}

	int day;
	try {
		day = Strings::toInt(parts[0]);
		if (day < 1 || day > 31) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid day: %d", day);
	}

	tm.tm_year = year;
	tm.tm_mon = month;
	tm.tm_mday = day;
}

// Format: HH:MM:SS
void Parser::parseTime(const std::string& time, std::tm& tm) const {
	auto parts = Strings::split(time, ":", 3);

	int hour;
	try {
		hour = Strings::toInt(parts[0]);
		if (hour < 0 || hour > 23) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid hour: %d", hour);
	}

	int min;
	try {
		min = Strings::toInt(parts[1]);
		if (min < 0 || min > 59) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid minute: %d", min);
	}

	int sec;
	try {
		sec = Strings::toInt(parts[2]);
		if (sec < 0 || sec > 59) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid second: %d", sec);
	}

	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
}

bool Parser::isPunctuation(char c) {
	return c == '?' || c == '!' || c == '.' || c == ',' || c == ';';
}

int Parser::offsetLine(int line) {
	return line - lineOffset_;
}

Parser::NodeRange::NodeRange(Node& node, const int min, const int max)
    : node{node}, min{min}, max{max} {}

Node* Parser::NodeRangeList::find(const int min, const int max) {
	Node* node = nullptr;

	for (const auto& nr : data) {
		if (min > nr.min) {
			if (max <= nr.max) {
				node = &nr.node;
			}
		} else {
			break;
		}
	}
	return node;
}

void Parser::NodeRangeList::add(Node& node, const int min, const int max) {
	data.emplace_back(node, min, max);
}

void Parser::NodeRangeList::clear() {
	data.clear();
}

Parser::DataHandler::DataHandler(
    std::size_t offset, std::size_t length, DataCallback func)
    : offset{offset}, length{length}, func{func} {}

Parser::LinkData::LinkData(const int targetLine, const int line, const int column)
    : targetLine{targetLine}, line{line}, column{column} {}

} // namespace UHS
