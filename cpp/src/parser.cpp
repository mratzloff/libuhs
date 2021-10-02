#include <climits>
#include <iostream>
#include <thread>

#include "uhs.h"

namespace UHS {

Parser::Parser(const Options options) : options_{options} {
	// TODO: Guard these by platform
	setenv("TZ", "", 1);
	tzset();
	// _putenv_s("TZ", ""); // MSVC
	// _tzset(); // MSVC
}

std::shared_ptr<Document> Parser::parse(std::istream& in) {
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

		if (!done_ && options_.mode != ModeType::Version88a) {
			// Set 88a header as first (hidden) child of 96a document
			document_->visibility(VisibilityType::None);

			// Create a document with a provisional 96a version
			// (updated once the version element is encountered)
			auto document = Document::create(VersionType::Version96a);
			document_.swap(document);
			document_->appendChild(document);

			this->parse96a();
		}

		thread.join();
		return std::move(document_);
	} catch (const Error& err) {
		thread.join();
		throw;
	}
}

std::shared_ptr<Document> Parser::parseFile(const std::string& infile) {
	std::filesystem::path infilePath{infile};

	if (!std::filesystem::exists(infilePath)) {
		throw ReadError("invalid file: %s", infile);
	}

	std::shared_ptr<Document> document;
	const auto size = std::filesystem::file_size(infilePath);
	std::ifstream in{infile, std::ios::in | std::ios::binary};

	if (size < 1'000'000) { // 1 MB
		std::stringstream buffer;
		buffer << in.rdbuf();
		in.close();
		document = this->parse(buffer);
	} else {
		document = this->parse(in);
	}

	return document;
}

void Parser::reset() {
	crc_.reset();
	dataHandlers_.clear();
	deferredLinks_.clear();
	document_.reset();
	done_ = false;
	isTitleSet_ = false;
	key_.clear();
	lineOffset_ = 0;
	parents_.clear();
	tokenizer_.reset();
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
	int firstHintTextLine = 0;
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
	int lastHintTextLine = 0;
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
// doesn't support it. It mostly works in the DOS reader, but it looks glitchy.
void Parser::parse88aElements(int firstHintTextLine, NodeMap& parents) {
	auto elementType = ElementType::Subject;

	for (;;) {
		auto token = this->expect(TokenType::String);
		std::string encodedTitle{token->value()};
		auto line = token->line();

		token = this->expect(TokenType::Line);
		int firstChildLine = 0;
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
		parent->appendChild(element);

		std::string title{codec_.decode88a(encodedTitle)};
		if (parent->nodeType() != NodeType::Document) {
			if (!Strings::endsWithFinalPunctuation(title)) {
				title += '?';
			}
		}
		if (options_.debug) {
			tfm::printf("\"%s\"\n", title);
		}
		e->title(title);

		firstChildLine = this->fixHeaderFirstChildLine(title, firstChildLine);

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

		auto element = static_cast<Element*>(parent);
		auto body = codec_.decode88a(token->value());
		if (options_.debug) {
			tfm::printf("\"%s\"\n", body);
		}

		auto group = GroupNode::create(line, 1);
		group->appendChild(TextNode::create(body));
		element->appendChild(group);
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

void Parser::parseHeaderSep(std::unique_ptr<const Token> token) {
	if (options_.mode == ModeType::Version88a) {
		done_ = true;
		return;
	}

	// 96a element line numbers don't include compatibility header
	lineOffset_ = token->line();
}

//--------------------------------- UHS 96a ---------------------------------//

void Parser::parse96a() {
	parents_.add(*document_, 0, INT_MAX);

	// Parse elements
	for (;;) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::Length:
			this->parseElement(std::move(token));
			break;
		case TokenType::Data:
			this->parseData(std::move(token));
			break;
		case TokenType::CRC:
			this->checkCRC();
			break;
		case TokenType::FileEnd:
			done_ = true; // i.e., done parsing
			goto done;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

done:
	this->processLinks();
	this->processVisibility();
}

// Elements are automatically appended to their parents
std::shared_ptr<Element> Parser::parseElement(std::unique_ptr<const Token> token) {
	// Length
	int length = 0;
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

	try {
		auto parent = this->findParent(*element);
		this->addNodeToParentIndex(*element);
		parent->appendChild(element);
	} catch (const Error& err) {
		std::throw_with_nested(ParseError(token->line(),
		    token->column(),
		    "orphaned element: %s",
		    element->elementTypeString()));
	}

	// Title
	token = this->expect(TokenType::String);
	element->title(codec_.decodeSpecialChars(token->value()));

	switch (elementType) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		this->parseCommentElement(*element);
		break;
	case ElementType::Credit:
		this->parseCommentElement(*element);
		break;
	case ElementType::Gifa:
		this->parseDataElement(*element);
		break;
	case ElementType::Hint:
		this->parseHintElement(*element);
		break;
	case ElementType::Hyperpng:
		this->parseHyperpngElement(*element);
		break;
	case ElementType::Incentive:
		this->parseIncentiveElement(*element);
		break;
	case ElementType::Info:
		this->parseInfoElement(*element);
		break;
	case ElementType::Link:
		this->parseLinkElement(*element);
		break;
	case ElementType::Nesthint:
		this->parseHintElement(*element);
		break;
	case ElementType::Overlay:
		this->parseOverlayElement(*element);
		break;
	case ElementType::Sound:
		this->parseDataElement(*element);
		break;
	case ElementType::Subject:
		this->parseSubjectElement(*element);
		break;
	case ElementType::Text:
		this->parseTextElement(*element);
		break;
	case ElementType::Version:
		this->parseVersionElement(*element);
		break;
	}

	return element;
}

void Parser::parseCommentElement(Element& element) {
	auto length = element.length();
	std::string body;
	auto continuation = false;
	auto paragraph = false;

	for (auto i = 3; i <= length; ++i) {
		auto token = this->next();

		switch (token->type()) {
		case TokenType::String:
			if (continuation) {
				body += ' ';
			}
			body += token->value();
			continuation = true;
			paragraph = false;
			break;
		case TokenType::NestedTextSep:
			[[fallthrough]];
		case TokenType::NestedParagraphSep:
			body += (paragraph) ? "\n" : "\n\n";
			continuation = false;
			paragraph = true;
			break;
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

	element.body(body);
}

void Parser::parseDataElement(Element& element) {
	std::size_t offset;
	auto line = 0;
	auto column = 0;

	for (;;) {
		auto token = this->next();
		line = token->line();
		column = token->column();

		switch (token->type()) {
		case TokenType::DataOffset: {
			int intOffset = 0;
			try {
				intOffset = Strings::toInt(token->value());
				if (intOffset < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(
				    line, column, ParseError::Uint, token->value());
			}
			offset = intOffset;
			goto expectDataLength;
		}
		case TokenType::TextFormat:
			continue; // Ignore
		default:
			throw ParseError::badToken(line, column, token->type());
		}
	}

expectDataLength:
	// Length
	auto token = this->expect(TokenType::DataLength);
	int intLength = 0;
	try {
		intLength = Strings::toInt(token->value());
		if (intLength < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(line, column, ParseError::Uint, token->value());
	}
	std::size_t length = intLength;

	// Data
	this->addDataCallback(line, column, offset, length, [=, &element](std::string data) {
		element.body(data);
	});
}

void Parser::parseHintElement(Element& element) {
	std::string body;
	int column = 0;
	std::shared_ptr<GroupNode> group;
	auto inlined = false;
	auto format = TextFormat::None;
	auto length = element.length();
	int line = 0;
	auto message = "could not parse formatted string";

	for (auto i = 3; i <= length; ++i) {
		auto token = this->next();
		line = token->line();
		column = token->column();

		if (!group) {
			group = GroupNode::create(this->offsetLine(line), length - i + 1);
			element.appendChild(group);
			this->addNodeToParentIndex(*group);
		}

		switch (token->type()) {
		case TokenType::String: {
			std::string text;

			if (element.elementType() == ElementType::Nesthint) {
				text = codec_.decode96a(token->value(), key_, false);
				if (text.starts_with(Token::InlineEnd)) {
					inlined = false;
				}
				if (text.ends_with(Token::InlineBegin)) {
					inlined = true;
				}
			} else {
				text = codec_.decode88a(token->value());
			}

			if (options_.debug) {
				tfm::printf("\"%s\"\n", text);
			}

			text += '\n';
			body += text;
			break;
		}
		case TokenType::NestedTextSep: {
			if (!body.empty()) {
				if (!body.ends_with("\n \n")) {
					Strings::chomp(body, '\n');
				}
				try {
					this->parseWithFormat(body, format, *group, element.elementType());
				} catch (const Error& err) {
					std::throw_with_nested(ParseError(line, column, message));
				}
				body.clear();
			}

			element.appendChild(BreakNode::create());

			if (group->hasFirstChild()) {
				group = GroupNode::create(this->offsetLine(line), length - i + 1);
				element.appendChild(group);
				this->addNodeToParentIndex(*group);
			}

			break;
		}
		case TokenType::NestedParagraphSep:
			body += " \n";
			break; // Handled by parseWithFormat()
		case TokenType::NestedElementSep: {
			if (i == length || element.elementType() != ElementType::Nesthint) {
				throw ParseError::badValue(
				    token->line(), token->column(), ParseError::String, token->value());
			}

			if (!body.empty()) {
				if (!body.ends_with("\n \n")) {
					Strings::chomp(body, '\n');
				}
				try {
					auto childFormat = TextFormat::None;
					this->parseWithFormat(
					    body, childFormat, *group, element.elementType());
				} catch (const Error& err) {
					std::throw_with_nested(ParseError(line, column, message));
				}
				body.clear();
			}

			// Length
			token = this->expect(TokenType::Length);

			// Child element
			const auto child = this->parseElement(std::move(token));
			i += child->length();

			if (inlined) {
				child->inlined(true);
			}

			// Add a trailing space to the previous child's text if necessary
			if (inlined && child->hasPreviousSibling()) {
				const auto previous = child->previousSibling();
				if (previous->isText()) {
					auto previousTextNode = static_cast<TextNode*>(previous);
					if (!Strings::endsWithAttachedPunctuation(previousTextNode->body())) {
						previousTextNode->body(previousTextNode->body() + " ");
					}
				}
			}

			break;
		}
		default:
			throw ParseError::badToken(token->line(), token->column(), token->type());
		}
	}

	if (!body.empty()) {
		try {
			this->parseWithFormat(body, format, *group, element.elementType());
		} catch (const Error& err) {
			std::throw_with_nested(ParseError(line, column, message));
		}
	}
}

void Parser::parseHyperpngElement(Element& element) {
	this->parseDataElement(element);

	// Parse child elements
	auto length = element.length();
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
		child->attr("region-top-left-x", x1);
		child->attr("region-top-left-y", y1);
		child->attr("region-bottom-right-x", x2);
		child->attr("region-bottom-right-y", y2);
	}
}

// A bad instruction ("12") is found at the beginning or end of the incentive
// list for four files, so we skip bad instructions of that form. Fourteen
// other files have lines pointing to nowhere, so we skip those, too.
void Parser::parseIncentiveElement(Element& element) {
	std::string body;

	element.visibility(VisibilityType::None);

	// Read and decode visibility instructions
	auto length = element.length();
	auto continuation = false;

	for (auto i = 2; i < length; ++i) {
		auto token = this->expect(TokenType::String);
		if (continuation) {
			body += ' ';
		}
		auto text = codec_.decode96a(token->value(), key_, false);
		if (options_.debug) {
			tfm::printf("\"%s\"\n", text);
		}
		body += text;
		continuation = true;
	}
	element.body(body);

	if (body.empty()) {
		return; // No instructions to process
	}

	// Process instructions
	auto instructions = Strings::split(body, " ");

	for (const auto& instruction : instructions) {
		auto instructionLength = instruction.length();

		// Split instruction (e.g., "3Z")
		if (instructionLength < 2) {
			continue; // TODO: Warn
		}

		int id = 0;
		try {
			id = Strings::toInt(instruction.substr(0, instructionLength - 1));
			if (id < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			continue; // TODO: Warn
		}

		auto flag = instruction.substr(instructionLength - 1, 1);

		VisibilityType visibility;
		if (flag == Token::RegisteredOnly) {
			visibility = VisibilityType::RegisteredOnly;
		} else if (flag == Token::UnregisteredOnly) {
			visibility = VisibilityType::UnregisteredOnly;
		} else {
			// TODO: Warn
			continue;
		}

		// Defer visibility processing to guarantee all nodes have been parsed.
		// Note that the incentive node always comes near the end, but it's
		// unclear if that's a specification requirement.
		deferredVisibilities_.emplace_back(id, visibility);
	}
}

void Parser::parseInfoElement(Element& element) {
	std::string serialized;
	std::string key;
	std::string value;
	std::string date;
	std::tm tm;

	element.visibility(VisibilityType::None);

	// Key-value pairs followed by a notice
	auto length = element.length();

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
			int fileLength = 0;
			try {
				fileLength = Strings::toInt(value);
				if (fileLength < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(
				    token->line(), token->column(), ParseError::Uint, value);
			}
			document_->attr("length", fileLength);
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

void Parser::parseLinkElement(Element& element) {
	// Ref line
	auto token = this->expect(TokenType::Line);
	element.body(token->value());
	deferredLinks_.emplace_back(element, token->line(), token->column());
}

void Parser::parseOverlayElement(Element& element) {
	this->parseDataElement(element);

	// Image top-left X coordinate
	auto token = this->expect(TokenType::CoordX);
	auto x = token->value();
	try {
		Strings::toInt(x);
	} catch (const Error& err) {
		throw ParseError::badValue(token->line(), token->column(), ParseError::Int, x);
	}
	element.attr("image-x", x);

	// Image bottom-right Y coordinate
	token = this->expect(TokenType::CoordY);
	auto y = token->value();
	try {
		Strings::toInt(y);
	} catch (const Error& err) {
		throw ParseError::badValue(token->line(), token->column(), ParseError::Int, y);
	}
	element.attr("image-y", y);
}

void Parser::parseSubjectElement(Element& element) {
	std::unique_ptr<const Token> token;

	if (!isTitleSet_) {
		document_->title(codec_.decodeSpecialChars(element.title()));
		key_ = codec_.createKey(element.title());
		isTitleSet_ = true;
	}

	const auto length = element.length();
	auto childLength = 0;

	for (auto i = 3; i <= length; i += childLength) {
		// Length
		auto token = this->expect(TokenType::Length);

		// Child element
		const auto child = this->parseElement(std::move(token));
		childLength = child->length();
	}
}

void Parser::parseTextElement(Element& element) {
	auto token = this->expect(TokenType::TextFormat);
	auto line = token->line();
	auto column = token->column();

	// Format
	int formatByte = 0;
	try {
		formatByte = Strings::toInt(token->value());
		if (formatByte < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(line, column, ParseError::Uint, token->value());
	}
	if (formatByte > 3) {
		throw ParseError(line,
		    column,
		    "text format byte must be between 0 and 3; found %d",
		    formatByte);
	}
	const auto format = static_cast<TextFormat>(formatByte);
	element.attr("format", (int) format);

	// Offset
	token = this->expect(TokenType::DataOffset);
	int intOffset = 0;
	try {
		intOffset = Strings::toInt(token->value());
		if (intOffset < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(line, column, ParseError::Uint, token->value());
	}
	std::size_t offset = intOffset;

	// Length
	token = this->expect(TokenType::DataLength);
	int intLength = 0;
	try {
		intLength = Strings::toInt(token->value());
		if (intLength < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(line, column, ParseError::Uint, token->value());
	}
	std::size_t length = intLength;

	// Data
	this->addDataCallback(line, column, offset, length, [=, &element](std::string data) {
		auto lines = Strings::split(data, EOL);
		for (auto& text : lines) {
			text = codec_.decode96a(text, key_, true);
			if (options_.debug) {
				tfm::printf("\"%s\"\n", text);
			}
		}

		auto body = Strings::rtrim(Strings::join(lines, "\n"), '\n');
		if (!body.empty()) {
			auto group = GroupNode::create(element.line(), element.length());
			element.appendChild(group);

			try {
				auto fmt = format;
				this->parseWithFormat(body, fmt, *group, element.elementType());
			} catch (const Error& err) {
				std::throw_with_nested(
				    ParseError(line, column, "could not parse formatted string"));
			}
		}
	});
}

void Parser::parseVersionElement(Element& element) {
	element.visibility(VisibilityType::None);
	this->parseCommentElement(element);

	auto version = element.title();
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

void Parser::addDataCallback(
    int line, int column, std::size_t offset, std::size_t length, DataCallback func) {

	dataHandlers_.emplace_back(line, column, offset, length, func);
}

void Parser::appendText(std::string& text, TextFormat& format, ContainerNode& node) {
	if (text.empty()) {
		return;
	}

	if (node.hasLastChild()) {
		auto previous = node.lastChild();

		if (previous->isText()) {
			auto previousTextNode = static_cast<TextNode*>(previous);
			auto previousTextBody = previousTextNode->body();

			// Append space to previous node if it does end in whitespace
			if (!previousTextBody.ends_with(' ') && !previousTextBody.ends_with('\n')
			    && !Strings::beginsWithAttachedPunctuation(text)) {

				previousTextBody += ' ';
				previousTextNode->body(previousTextBody);
			}

			// Append text with the same formatting to the previous node
			if (previousTextNode->format() == format) {
				previousTextBody += text;
				previousTextNode->body(previousTextBody);

				text.clear();
				return;
			}
		} else if (previous->isElement()) {
			const auto previousElement = static_cast<Element*>(previous);

			// Prepend space to text if it follows an inline element
			if (previousElement->inlined()
			    && !Strings::beginsWithAttachedPunctuation(text)) {

				text = " " + text;
			}
		}
	}

	auto textNode = TextNode::create(text, format);
	node.appendChild(textNode);
	text.clear();
}

void Parser::checkCRC() {
	document_->validChecksum(crc_->valid());
}

Node* Parser::findParent(ContainerNode& node) {
	auto min = node.line();
	auto max = min + node.length();
	auto parent = parents_.find(min, max);

	if (!parent) {
		throw Error("could not find parent element between lines %d and %d", min, max);
	}

	return parent;
}

// Most UHS files have a header with an off-by-one error in the (invisible) 88a header.
// This is a small fix to address that should someone use --mode=88a.
int Parser::fixHeaderFirstChildLine(std::string title, int firstChildLine) const {
	if (title == BadHeaderTitle && firstChildLine == BadHeaderFirstChildLine) {
		return GoodHeaderFirstChildLine;
	}
	return firstChildLine;
}

void Parser::addNodeToParentIndex(ContainerNode& node) {
	auto min = node.line();
	auto max = min + node.length();
	parents_.add(node, min, max);
}

int Parser::offsetLine(int line) {
	return line - lineOffset_;
}

void Parser::parseData(std::unique_ptr<const Token> token) {
	std::size_t offset;

	for (const auto& handler : dataHandlers_) {
		offset = handler.offset - token->offset();
		handler.func(token->value().substr(offset, handler.length));
	}
}

// Format: DD-Mon-YY
void Parser::parseDate(const std::string& date, std::tm& tm) const {
	auto parts = Strings::split(date, "-", 3);

	int year = 0;
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

	int month = 0;
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

	int day = 0;
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

	int hour = 0;
	try {
		hour = Strings::toInt(parts[0]);
		if (hour < 0 || hour > 23) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid hour: %d", hour);
	}

	int min = 0;
	try {
		min = Strings::toInt(parts[1]);
		if (min < 0 || min > 59) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid minute: %d", min);
	}

	int sec = 0;
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

void Parser::parseWithFormat(const std::string& text, TextFormat& format,
    ContainerNode& node, ElementType elementType) {

	const auto s = codec_.decodeSpecialChars(text);
	auto length = s.length();
	std::string segment;

	for (std::size_t i = 0; i < length; ++i) {
		switch (s[i]) { // TODO: Move this to a function map once logic is accurate
		case '\n':
			if (s.substr(i + 1, 2) == " \n") {
				// Handle paragraph breaks
				for (std::size_t j = i; j < length && s.substr(j + 1, 2) == " \n";
				     j += 2, i = j) {

					segment += '\n';
				}
				segment += '\n';

			} else if (elementType == ElementType::Text
			           || hasFormat(format, TextFormat::Monospace)
			           || hasFormat(format, TextFormat::Overflow)) {
				// Preserve newlines for text elements and relevant formats
				segment += s[i];
			} else {
				// Find the preceding newline
				auto pos = s.find_last_of('\n', i - 1);
				if (pos == std::string::npos) {
					pos = -1;
				}

				if (s.substr(pos + 1, 2) == "  ") {
					// Preserve any newline at the end of an indented line
					segment += s[i];
				} else {
					this->appendText(segment, format, node);
				}
			}
			break;
		case Token::Escape:
			if (i + 1 < length && s[i + 1] == Token::Escape) {
				i += 1;
				segment += '#';
			} else if (i + 2 < length) {
				switch (s[i + 1]) {
				case 'a':
					segment += s[i];
					break;
				case 'h':
					switch (s[i + 2]) {
					case '+':
						if (hasFormat(format, TextFormat::Hyperlink)) {
							// TODO: Warn: unexpected sequence
						}

						this->appendText(segment, format, node);
						format = withFormat(format, TextFormat::Hyperlink);
						i += 2;
						break;
					case '-':
						if (!hasFormat(format, TextFormat::Hyperlink)) {
							// TODO: Warn: unexpected sequence
						}

						this->appendText(segment, format, node);
						format = withoutFormat(format, TextFormat::Hyperlink);
						i += 2;
						break;
					default:
						// TODO: Warn: unexpected sequence
						break;
					}
					break;
				case 'p':
					switch (s[i + 2]) {
					case '-':
						if (hasFormat(format, TextFormat::Monospace)) {
							// TODO: Warn: unexpected sequence
						}

						this->appendText(segment, format, node);
						format = withFormat(format, TextFormat::Monospace);
						i += 2;
						break;
					case '+':
						if (!hasFormat(format, TextFormat::Monospace)) {
							// TODO: Warn: unexpected sequence
						}

						this->appendText(segment, format, node);
						format = withoutFormat(format, TextFormat::Monospace);
						i += 2;
						break;
					default:
						break; // TODO: Warn
					}
					break;
				case 'w':
					switch (s[i + 2]) {
					case '-':
						this->appendText(segment, format, node);
						format = withFormat(format, TextFormat::Overflow);
						i += 2;
						break;
					case '+':
						this->appendText(segment, format, node);
						format = withoutFormat(format, TextFormat::Overflow);
						i += 2;
						break;
					case '.':
						this->appendText(segment, format, node);
						format = withoutFormat(format, TextFormat::Overflow);
						i += 2;
						break;
					default:
						// TODO: Warn
						break;
					}
					break;
				default:
					// TODO: Warn
					break;
				}
			} else {
				// TODO: Warn
			}
			break;
		default:
			segment += s[i];
		}
	}

	this->appendText(segment, format, node);
}

void Parser::processLinks() {
	for (const auto& [link, line, column] : deferredLinks_) {
		const auto& body = link.body();
		int targetLine = 0;
		try {
			targetLine = Strings::toInt(body);
			if (targetLine < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			throw ParseError::badValue(line, column, ParseError::Uint, body);
		}

		Node* target = nullptr;
		if (target = document_->find(targetLine); !target) {
			// A link pointing to the child of a hyperpng element is a special
			// case. Instead of referencing the descriptor line, it points to the
			// region, even if it doesn't share a parent hyperpng. A link outside
			// of a hyperpng pointing to a child of a hyperpng references the
			// descriptor line, as usual.
			//
			// The most straightforward way of maintaining a consistent ID model
			// is to modify the link body. However, we leave a trail for debugging
			// purposes.
			if (target = document_->find(targetLine + 1); target) {
				if (const auto parent = target->parent()) {
					const auto parentElement = static_cast<Element*>(parent);
					if (parentElement->elementType() == ElementType::Hyperpng) {
						link.body(targetLine + 1);
						link.attr("unmodified-target", targetLine);
					} else {
						target = nullptr; // We found an element, but shouldn't have
					}
				}
			}
		}
		if (!target) {
			// TODO: Warn using line and column
			throw ParseError(line, column, "target not found: %d", targetLine);
		}
	}
}

void Parser::processVisibility() {
	for (auto [targetLine, visibility] : deferredVisibilities_) {
		if (auto target = document_->find(targetLine)) {
			if (!target->isElement()) {
				// TODO: Warn
				continue;
			}

			auto targetElement = static_cast<Element*>(target);
			targetElement->visibility(visibility);

			if (!options_.preserve) {
				switch (visibility) {
				case VisibilityType::RegisteredOnly:
					targetElement->visibility(VisibilityType::All);
					break;
				case VisibilityType::UnregisteredOnly: {
					auto node = target->pointer();
					if (!target->hasParent() || !node) {
						throw Error("cannot remove orphaned node");
					}
					target->parent()->removeChild(node);
					break;
				}
				default:
					break; // No change
				}
			}
		} else {
			// TODO: Warn
		}
	}
}

Parser::NodeRange::NodeRange(Node& node, const int min, const int max)
    : node{node}, min{min}, max{max} {}

Node* Parser::NodeRangeList::find(const int min, const int max) {
	Node* node = nullptr;

	for (const auto& each : data) {
		if (min > each.min) {
			if (max <= each.max) {
				node = &each.node;
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
    int line, int column, std::size_t offset, std::size_t length, DataCallback func)
    : line{line}, column{column}, offset{offset}, length{length}, func{func} {}

Parser::LinkData::LinkData(Element& link, const int line, const int column)
    : link{link}, line{line}, column{column} {}

Parser::VisibilityData::VisibilityData(int targetLine, VisibilityType visibility)
    : targetLine{targetLine}, visibility{visibility} {}

} // namespace UHS
