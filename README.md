# libuhs

## Build

First, install the following prerequisites:

-   [CMake](https://cmake.org), a build system for C and C++
-   [Node.js](https://nodejs.org), a JavaScript runtime
-   [Yarn](https://yarnpkg.com), a JavaScript package manager

Afterward, you just need to run the following from the project root to build:

`cmake . && make`

This will install several remote C, C++, and TypeScript dependencies, then build the library and binaries.

## Elements

### gifa

Only used by dejavu.uhs.

### nesthint

Elements that a nesthint element can contain:

-   hint
-   hyperpng
-   link
-   nesthint
-   sound
-   subject
-   text

All of these can be rendered inline.

### sound

Only used by overseer.uhs.

## Formatting token thoughts

Bioshock, Bioshock 2, Fallout: New Vegas, Neverwinter Nights 2, and a handful of other files use all of the following features.

### #a

Used to denote certain non-ASCII characters. Added in version 2002a, which retained the 96a version in files. (Found in 46 files.)

-   `#a+` Start non-ASCII sequence.
-   `#a-` End non-ASCII sequence.

### #h

Used to denote a hyperlink, either a URL (may not include protocol) or an email address. Added in version 96a. (Found in 68 files.)

-   `#h+` Start hyperlink sequence.
-   `#h-` End hyperlink sequence.
-   `#h+\nhttp://example.com#h-` Text can span multiple lines and may contain whitespace before or after the link.

### #p

Used to denote proportional typeface, typically to disable the default in favor of monospace. Added in version 96a. (Found in 126 files.)

-   `#p-` Start a monospace typeface sequence.
-   `#p+` End a monospace typeface sequence.

### #w

Used to denote word wrap, or more specifically to honor newlines. As word wrap is the default for UHS files, this is implemented as the inverse (overflow) instead. Added in version 96a.

-   `#w-` Honor newlines. A single whitespace character between tokens is also used to break a long string of text (for example, `#w-\n#w+`). (Found in 190 files.)
-   `#w+` Ignore newlines and wrap all subsequent characters into a single line. In nesthint and text elements, this sequence also indicates the following nested element should be displayed inline. Finally, a single whitespace character between tokens is also used to force a long string of text, typically in a table or list (for example, `#w+\n#w-` or `#w+ #w-`). (Found in 310 files.)
-   `#w.` Honor newlines. Used to indicate the end of an inline link. (Found in 296 files.)

## Other observed behavior

-   At some early point, lines beginning with two spaces were apparently considered part of a preformatted block. The current reader still applies this rule, but only to lines of no more than 20 characters. You can see several examples of this in 11thhour.uhs.

## Known issues

-   UHSWriter: In certain rare cases, text that should overflow (that is, be on one contiguous line and cause horizontal scroll if the viewport is too narrow) instead wraps at hard cut lines. The "Annotated Map Key" entries in oblivion.uhs are good examples of this behavior. While this bug is possible to fix, it's not trivial; it requires overhauling the complex logic of how line wrapping works for formatted text nodes (line wrapping being core to UHS, a line-based format). Since UHS is a dead format and I only built the writing capability to thoroughly understand the format via black-box testing, it's not a high priority for me.

## Things that may seem like bugs but aren't

-   Text that is intended to overflow the container is correctly parsed and written, but the HTML output intentionally ignores the overflow property on display.

-   The backwards compatibility header included in most UHS files shows the first line of "Why aren't there any more hints here?" as the last line of "Ignore this!". This is a bug in the header itself, and a small fix is applied to adjust for this.

-   By default, sq4g.uhs does not parse correctly; unexpected newline characters break hint parsing. This appears to be the result of a miscompiled file. It also fails in the official reader and is unavailable on the official website.

-   There are also bad incentive values and bad link targets in various files, but the library should parse all of these without an issue and only issue a warning.

## Known issues in official readers

### UHS Reader for Windows 6.10

-   There's a bug with the gifa block in dejavu.uhs; the map will not render. GIF rendering was probably removed as a result of Unisys enforcing LZW patents in the late 1990s. (All Unisys patents related to GIF expired in 2004.)

### uhs-hints.com

-   All the same issues for the Windows reader apply to the website, although gifa is probably better considered missing functionality; a message notifies users that it is unimplemented.

-   The website incorrectly renders monospace text blocks that word wrap within the container (text format byte 1) as monospace text blocks that overflow the container (text format byte 3).
