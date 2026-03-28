import { beforeEach, describe, expect, it, vi } from "vitest";

import Viewport from "../src/viewport";

/**
 * Comprehensive fixture matching real HTML writer output.
 *
 *   document (96a)
 *   ├── [hidden] document (88a, visibility=none)
 *   │   └── subject#header "–"
 *   │       └── hint#header-hint "–?"
 *   │           └── "–"
 *   ├── subject#1 "Shadowed Passage Atlas"              ← HOME
 *   │   ├── subject#3 "The Cellar"
 *   │   │   ├── credit#5 "Who wrote this section?"
 *   │   │   │   body: "Compiled by River Ashford."
 *   │   │   ├── nesthint#8 "How do I open the trapdoor?"
 *   │   │   │   ├── "Look for scratches on the floor."
 *   │   │   │   ├── "Pull the iron ring."
 *   │   │   │   └── "It leads to the crypt."
 *   │   │   ├── blank#13
 *   │   │   ├── hint#15 "Is there a key?"
 *   │   │   │   └── "Under the loose flagstone."
 *   │   │   └── link#18 → #21 "See also: The Tower"
 *   │   ├── subject#21 "The Tower"
 *   │   │   ├── nesthint#23 "How do I reach the top?"
 *   │   │   │   ├── "Take the spiral staircase."
 *   │   │   │   └── "Watch for the collapsed step."
 *   │   │   ├── text#28 "Tower layout"
 *   │   │   │   body: "The tower has three floors."
 *   │   │   └── link#31 → #3 "See also: The Cellar"
 *   │   └── nesthint#34 "FAQ"
 *   │       ├── "Read the manual."
 *   │       └── "Ask the innkeeper."
 *   ├── [hidden] version#39 (visibility=none)
 *   └── [hidden] info#41 (visibility=none)
 */
function buildFixture(): void {
    document.body.innerHTML = `
<main id="root" hidden="">
<section data-type="document" data-version="96a">
<ol>
<li hidden="">
<section data-type="document" data-version="88a" data-visibility="none">
<ol>
<li>
<div data-id="99" data-type="subject">
<div><span class="title">-</span></div>
<ol>
<li>
<div data-id="98" data-type="hint">
<div><span class="title">-?</span></div>
<ol>
<li><div class="hint"><p><span>-</span></p></div></li>
</ol>
</div>
</li>
</ol>
</div>
</li>
</ol>
</section>
</li>
<li>
<div data-id="1" id="1" data-type="subject">
<div><span class="title">Shadowed Passage Atlas</span></div>
<ol>
<li>
<div data-id="3" id="3" data-type="subject">
<div><span class="title">The Cellar</span></div>
<ol>
<li>
<div data-id="5" id="5" data-type="credit">
<div><span class="title">Who wrote this section?</span></div>
<p>Compiled by River Ashford.</p>
</div>
</li>
<li>
<div data-id="8" id="8" data-type="nesthint">
<div><span class="title">How do I open the trapdoor?</span></div>
<ol>
<li><div class="hint"><p><span>Look for scratches on the floor.</span></p></div></li>
<li><div class="hint"><p><span>Pull the iron ring.</span></p></div></li>
<li><div class="hint"><p><span>It leads to the crypt.</span></p></div></li>
</ol>
</div>
</li>
<li>
<hr data-id="13" id="13" data-type="blank" />
</li>
<li>
<div data-id="15" id="15" data-type="hint">
<div><span class="title">Is there a key?</span></div>
<ol>
<li><div class="hint"><p><span>Under the loose flagstone.</span></p></div></li>
</ol>
</div>
</li>
<li>
<a data-id="18" id="18" data-type="link" data-target="21" href="#21">See also: The Tower</a>
</li>
</ol>
</div>
</li>
<li>
<div data-id="21" id="21" data-type="subject">
<div><span class="title">The Tower</span></div>
<ol>
<li>
<div data-id="23" id="23" data-type="nesthint">
<div><span class="title">How do I reach the top?</span></div>
<ol>
<li><div class="hint"><p><span>Take the spiral staircase.</span></p></div></li>
<li><div class="hint"><p><span>Watch for the collapsed step.</span></p></div></li>
</ol>
</div>
</li>
<li>
<div data-id="28" id="28" data-type="text">
<div><span class="title">Tower layout</span></div>
<p>The tower has three floors.</p>
</div>
</li>
<li>
<a data-id="31" id="31" data-type="link" data-target="3" href="#3">See also: The Cellar</a>
</li>
</ol>
</div>
</li>
<li>
<div data-id="34" id="34" data-type="nesthint">
<div><span class="title">FAQ</span></div>
<ol>
<li><div class="hint"><p><span>Read the manual.</span></p></div></li>
<li><div class="hint"><p><span>Ask the innkeeper.</span></p></div></li>
</ol>
</div>
</li>
</ol>
</div>
</li>
<li hidden="">
<div data-id="39" id="39" data-type="version" data-visibility="none">
<div><span class="title">96a</span></div>
</div>
</li>
<li hidden="">
<div data-id="41" id="41" data-type="info" data-visibility="none" id="info">
<dl><dt>Date Created</dt><dd>2026-03-20</dd></dl>
</div>
</li>
</ol>
</section>
</main>`;
}

// ── Helpers ──────────────────────────────────────────────────────────

function getViewport(): HTMLElement {
    return document.getElementById("viewport")!;
}

function getHeading(): string | null {
    return getViewport().querySelector("h1")?.textContent ?? null;
}

function getTitles(): string[] {
    return Array.from(getViewport().querySelectorAll(".title")).map(
        element => element.textContent ?? "",
    );
}

function getClickableTitles(): string[] {
    return Array.from(getViewport().querySelectorAll(".title.clickable")).map(
        element => element.textContent ?? "",
    );
}

function getLinks(): string[] {
    return Array.from(getViewport().querySelectorAll("a.clickable")).map(
        element => element.textContent ?? "",
    );
}

function getVisibleHints(): string[] {
    const items = getViewport().querySelectorAll("ol > li");

    return Array.from(items)
        .filter(item => !(item as HTMLElement).hidden)
        .map(item => item.textContent?.trim() ?? "");
}

function getHiddenHintCount(): number {
    const items = getViewport().querySelectorAll("ol > li");

    return Array.from(items).filter(item => (item as HTMLElement).hidden)
        .length;
}

function getFooterButtonText(): string | null {
    return document.getElementById("button")?.textContent ?? null;
}

function getBreadcrumbTexts(): string[] {
    const breadcrumbs = getViewport().querySelector("ul.breadcrumbs");
    if (!breadcrumbs) {
        return [];
    }

    return Array.from(breadcrumbs.querySelectorAll("li")).map(
        li => li.textContent ?? "",
    );
}

function hasBlankElement(): boolean {
    return getViewport().querySelector("hr") !== null;
}

function clickTitle(text: string): void {
    const title = Array.from(
        getViewport().querySelectorAll(".title.clickable"),
    ).find(element => element.textContent === text);
    (title as HTMLElement)?.click();
}

function clickLink(text: string): void {
    const link = Array.from(getViewport().querySelectorAll("a.clickable")).find(
        element => element.textContent === text,
    );
    (link as HTMLElement)?.click();
}

function clickFooterButton(): void {
    document.getElementById("button")?.click();
}

function clickButton(id: string): void {
    document.getElementById(id)?.click();
}

// ── Tests ────────────────────────────────────────────────────────────

describe("Viewport", () => {
    beforeEach(() => {
        document.body.innerHTML = "";
        sessionStorage.clear();
        vi.stubGlobal("scrollTo", vi.fn());
    });

    // ── initOnReady ──────────────────────────────────────────────────

    describe("initOnReady", () => {
        it("constructs Viewport on DOMContentLoaded", () => {
            Viewport.initOnReady();
            expect(document.getElementById("viewport")).toBeNull();

            buildFixture();
            document.dispatchEvent(new Event("DOMContentLoaded"));
            expect(document.getElementById("viewport")).not.toBeNull();
        });
    });

    // ── Construction ─────────────────────────────────────────────────

    describe("construction", () => {
        it("creates nav with buttons", () => {
            buildFixture();
            new Viewport();

            const nav = document.getElementById("nav");
            expect(nav).not.toBeNull();
            expect(document.getElementById("back")).not.toBeNull();
            expect(document.getElementById("forward")).not.toBeNull();
            expect(document.getElementById("home")).not.toBeNull();
        });

        it("creates viewport element", () => {
            buildFixture();
            new Viewport();

            expect(getViewport()).not.toBeNull();
        });

        it("creates options menu with table toggle", () => {
            buildFixture();
            new Viewport();

            const optionsMenu = document.getElementById("options-menu");
            expect(optionsMenu).not.toBeNull();

            const toggle = optionsMenu?.querySelector("input[type='checkbox']");
            expect(toggle).not.toBeNull();
        });

        it("disables back and forward buttons initially", () => {
            buildFixture();
            new Viewport();

            expect(
                document.getElementById("back")?.hasAttribute("disabled"),
            ).toBe(true);
            expect(
                document.getElementById("forward")?.hasAttribute("disabled"),
            ).toBe(true);
        });

        it("shows viewport after rendering", () => {
            buildFixture();
            new Viewport();

            expect(getViewport().style.display).toBe("block");
        });

        it("scrolls to top on construction", () => {
            buildFixture();
            new Viewport();

            expect(scrollTo).toHaveBeenCalledWith(0, 0);
        });
    });

    // ── Home (#1) ────────────────────────────────────────────────────

    describe("home view", () => {
        it("shows the document title as heading", () => {
            buildFixture();
            new Viewport();

            expect(getHeading()).toBe("Shadowed Passage Atlas");
        });

        it("shows exactly three child titles", () => {
            buildFixture();
            new Viewport();

            expect(getClickableTitles()).toEqual([
                "The Cellar",
                "The Tower",
                "FAQ",
            ]);
        });

        it("does not show hidden elements (version, info)", () => {
            buildFixture();
            new Viewport();

            const titles = getTitles();
            expect(titles).not.toContain("96a");
        });

        it("does not show the 88a header document", () => {
            buildFixture();
            new Viewport();

            const titles = getTitles();
            expect(titles).not.toContain("-");
            expect(titles).not.toContain("-?");
        });

        it("has no footer", () => {
            buildFixture();
            new Viewport();

            expect(getFooterButtonText()).toBeNull();
        });

        it("has no breadcrumbs", () => {
            buildFixture();
            new Viewport();

            expect(getBreadcrumbTexts()).toEqual([]);
        });
    });

    // ── The Cellar (#3) ──────────────────────────────────────────────

    describe("The Cellar (subject with mixed children)", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("The Cellar");
        });

        it("shows credit, nesthint, hint, and link titles", () => {
            expect(getClickableTitles()).toEqual([
                "Who wrote this section?",
                "How do I open the trapdoor?",
                "Is there a key?",
            ]);
        });

        it("shows cross-reference link", () => {
            expect(getLinks()).toEqual(["See also: The Tower"]);
        });

        it("shows blank separator", () => {
            expect(hasBlankElement()).toBe(true);
        });

        it("has no footer", () => {
            expect(getFooterButtonText()).toBeNull();
        });

        it("has no breadcrumbs for direct child of home", () => {
            expect(getBreadcrumbTexts()).toEqual([]);
        });
    });

    // ── The Tower (#21) ──────────────────────────────────────────────

    describe("The Tower (subject with text and link)", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Tower");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("The Tower");
        });

        it("shows nesthint and text titles as clickable", () => {
            expect(getClickableTitles()).toEqual([
                "How do I reach the top?",
                "Tower layout",
            ]);
        });

        it("shows cross-reference link back to The Cellar", () => {
            expect(getLinks()).toEqual(["See also: The Cellar"]);
        });

        it("does not show blank separator", () => {
            expect(hasBlankElement()).toBe(false);
        });
    });

    // ── Nesthint: How do I open the trapdoor? (#8) ──────────────────

    describe("nesthint with three hints", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickTitle("How do I open the trapdoor?");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("How do I open the trapdoor?");
        });

        it("shows only first hint initially", () => {
            expect(getVisibleHints()).toEqual([
                "Look for scratches on the floor.",
            ]);
        });

        it("hides remaining hints", () => {
            expect(getHiddenHintCount()).toBe(2);
        });

        it("shows Show next hint button", () => {
            expect(getFooterButtonText()).toBe("Show next hint");
        });

        it("reveals second hint on button click", () => {
            clickFooterButton();

            expect(getVisibleHints()).toEqual([
                "Look for scratches on the floor.",
                "Pull the iron ring.",
            ]);
        });

        it("reveals all hints after clicking twice", () => {
            clickFooterButton();
            clickFooterButton();

            expect(getVisibleHints()).toEqual([
                "Look for scratches on the floor.",
                "Pull the iron ring.",
                "It leads to the crypt.",
            ]);
        });

        it("changes button to Back on last hint", () => {
            clickFooterButton();
            clickFooterButton();

            expect(getFooterButtonText()).toBe("Back");
        });

        it("shows breadcrumbs with parent subject", () => {
            expect(getBreadcrumbTexts()).toEqual(["The Cellar"]);
        });

        it("navigates back to The Cellar on Back click", () => {
            clickFooterButton();
            clickFooterButton();
            clickFooterButton();

            expect(getHeading()).toBe("The Cellar");
        });
    });

    // ── Hint: Is there a key? (#15) ─────────────────────────────────

    describe("hint with single hint", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickTitle("Is there a key?");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("Is there a key?");
        });

        it("shows the single hint", () => {
            expect(getVisibleHints()).toEqual(["Under the loose flagstone."]);
        });

        it("shows Back button immediately for single hint", () => {
            expect(getFooterButtonText()).toBe("Back");
        });

        it("shows breadcrumbs", () => {
            expect(getBreadcrumbTexts()).toEqual(["The Cellar"]);
        });
    });

    // ── Credit: Who wrote this section? (#5) ─────────────────────────

    describe("credit element (leaf node)", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickTitle("Who wrote this section?");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("Who wrote this section?");
        });

        it("shows body text", () => {
            const viewport = getViewport();
            expect(viewport.textContent).toContain(
                "Compiled by River Ashford.",
            );
        });

        it("has no footer (not a hint)", () => {
            expect(getFooterButtonText()).toBeNull();
        });

        it("shows breadcrumbs", () => {
            expect(getBreadcrumbTexts()).toEqual(["The Cellar"]);
        });
    });

    // ── Text: Tower layout (#28) ─────────────────────────────────────

    describe("text element (leaf node)", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("The Tower");
            clickTitle("Tower layout");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("Tower layout");
        });

        it("shows body text", () => {
            const viewport = getViewport();
            expect(viewport.textContent).toContain(
                "The tower has three floors.",
            );
        });

        it("has no footer", () => {
            expect(getFooterButtonText()).toBeNull();
        });

        it("shows breadcrumbs", () => {
            expect(getBreadcrumbTexts()).toEqual(["The Tower"]);
        });
    });

    // ── FAQ (#34) (nesthint directly under home) ─────────────────────

    describe("FAQ nesthint (direct child of home)", () => {
        beforeEach(() => {
            buildFixture();
            new Viewport();
            clickTitle("FAQ");
        });

        it("shows heading", () => {
            expect(getHeading()).toBe("FAQ");
        });

        it("shows first hint only", () => {
            expect(getVisibleHints()).toEqual(["Read the manual."]);
        });

        it("reveals second hint", () => {
            clickFooterButton();

            expect(getVisibleHints()).toEqual([
                "Read the manual.",
                "Ask the innkeeper.",
            ]);
        });

        it("has no breadcrumbs (direct child of home)", () => {
            expect(getBreadcrumbTexts()).toEqual([]);
        });
    });

    // ── Cross-reference links ────────────────────────────────────────

    describe("cross-reference navigation", () => {
        it("navigates from The Cellar to The Tower via link", () => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickLink("See also: The Tower");

            expect(getHeading()).toBe("The Tower");
        });

        it("navigates from The Tower to The Cellar via link", () => {
            buildFixture();
            new Viewport();
            clickTitle("The Tower");
            clickLink("See also: The Cellar");

            expect(getHeading()).toBe("The Cellar");
        });

        it("shows correct breadcrumbs after link navigation", () => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickLink("See also: The Tower");

            // The Tower is a direct child of home, so no breadcrumbs
            expect(getBreadcrumbTexts()).toEqual([]);
        });

        it("maintains history through link navigation", () => {
            buildFixture();
            new Viewport();
            clickTitle("The Cellar");
            clickLink("See also: The Tower");
            clickButton("back");

            expect(getHeading()).toBe("The Cellar");
        });
    });

    // ── History through multi-level navigation ───────────────────────

    describe("deep navigation and history", () => {
        it("maintains full history chain", () => {
            buildFixture();
            new Viewport();

            // Home → The Cellar → How do I open the trapdoor?
            clickTitle("The Cellar");
            clickTitle("How do I open the trapdoor?");

            expect(getHeading()).toBe("How do I open the trapdoor?");

            // Back to The Cellar
            clickButton("back");
            expect(getHeading()).toBe("The Cellar");

            // Back to Home
            clickButton("back");
            expect(getHeading()).toBe("Shadowed Passage Atlas");

            // Forward to The Cellar
            clickButton("forward");
            expect(getHeading()).toBe("The Cellar");

            // Forward to the trapdoor hint
            clickButton("forward");
            expect(getHeading()).toBe("How do I open the trapdoor?");
        });

        it("truncates forward history on new navigation", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            clickTitle("How do I open the trapdoor?");
            clickButton("back");
            clickButton("back");

            // Now navigate somewhere else
            clickTitle("The Tower");
            expect(getHeading()).toBe("The Tower");

            // Forward should be disabled (history truncated)
            expect(
                document.getElementById("forward")?.hasAttribute("disabled"),
            ).toBe(true);
        });
    });

    // ── Hint cache across navigation ─────────────────────────────────

    describe("hint cache persistence", () => {
        it("remembers revealed hints when navigating away and back", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            clickTitle("How do I open the trapdoor?");

            // Reveal second hint
            clickFooterButton();
            expect(getVisibleHints()).toHaveLength(2);

            // Navigate away and back
            clickButton("back");
            clickTitle("How do I open the trapdoor?");

            expect(getVisibleHints()).toHaveLength(2);
            expect(getVisibleHints()).toEqual([
                "Look for scratches on the floor.",
                "Pull the iron ring.",
            ]);
        });

        it("remembers full reveal state", () => {
            buildFixture();
            new Viewport();

            clickTitle("FAQ");

            // Reveal all hints
            clickFooterButton();
            expect(getVisibleHints()).toHaveLength(2);

            // Navigate away and back
            clickButton("back");
            clickTitle("FAQ");

            expect(getVisibleHints()).toHaveLength(2);
            expect(getFooterButtonText()).toBe("Back");
        });
    });

    // ── Search ───────────────────────────────────────────────────────

    describe("search", () => {
        it("renders search results on Enter key", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "Cellar";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "Enter" }),
            );

            const heading = getViewport().querySelector("h1");
            expect(heading?.textContent).toBe("Search: Cellar");
        });

        it("does not search on non-Enter key", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "Cellar";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "a" }),
            );

            const heading = getViewport().querySelector("h1");
            expect(heading?.textContent).toBe("Shadowed Passage Atlas");
        });

        it("does not search for empty query", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "   ";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "Enter" }),
            );

            const heading = getViewport().querySelector("h1");
            expect(heading?.textContent).toBe("Shadowed Passage Atlas");
        });

        it("shows no results message when nothing matches", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "nonexistent";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "Enter" }),
            );

            expect(getViewport().textContent).toContain("No results found");
        });

        it("shows matching results as clickable links", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "Tower";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "Enter" }),
            );

            const results = getViewport().querySelectorAll(
                "ol.search-results li",
            );
            expect(results.length).toBeGreaterThanOrEqual(2);
        });

        it("navigates to hint when clicking search result", () => {
            buildFixture();
            new Viewport();

            const searchField = document.querySelector(
                "input[type='search']",
            ) as HTMLInputElement;
            searchField.value = "Cellar";
            searchField.dispatchEvent(
                new KeyboardEvent("keydown", { key: "Enter" }),
            );

            const result = getViewport().querySelector(
                "ol.search-results .title.clickable",
            ) as HTMLElement;
            result?.click();

            const heading = getViewport().querySelector("h1");
            expect(heading?.textContent).toBe("The Cellar");
        });
    });

    // ── Table mode toggle ────────────────────────────────────────────

    describe("table mode toggle", () => {
        it("defaults to text mode", () => {
            buildFixture();
            new Viewport();

            const toggle = document.querySelector(
                "#options-menu input[type='checkbox']",
            ) as HTMLInputElement;
            expect(toggle.checked).toBe(false);
        });

        it("restores table mode from session storage", () => {
            sessionStorage.setItem("option.table.mode", "html");
            buildFixture();
            new Viewport();

            const toggle = document.querySelector(
                "#options-menu input[type='checkbox']",
            ) as HTMLInputElement;
            expect(toggle.checked).toBe(true);
        });

        it("persists table mode to session storage on toggle", () => {
            buildFixture();
            new Viewport();

            const toggle = document.querySelector(
                "#options-menu input[type='checkbox']",
            ) as HTMLInputElement;
            toggle.checked = true;
            toggle.dispatchEvent(new Event("change"));

            expect(sessionStorage.getItem("option.table.mode")).toBe("html");
        });
    });

    // ── Options menu ─────────────────────────────────────────────────

    describe("options menu", () => {
        it("toggles options menu on button click", () => {
            buildFixture();
            new Viewport();

            clickButton("options");

            const menu = document.getElementById("options-menu");
            expect(menu?.classList.contains("open")).toBe(true);
        });

        it("closes options menu on second click", () => {
            buildFixture();
            new Viewport();

            clickButton("options");
            clickButton("options");

            const menu = document.getElementById("options-menu");
            expect(menu?.classList.contains("open")).toBe(false);
        });
    });

    // ── window.uhs API ──────────────────────────────────────────────

    describe("window.uhs", () => {
        it("exposes back function", () => {
            buildFixture();
            new Viewport();

            expect(window.uhs).toBeDefined();
            expect(typeof window.uhs.back).toBe("function");
        });

        it("exposes forward function", () => {
            buildFixture();
            new Viewport();

            expect(typeof window.uhs.forward).toBe("function");
        });

        it("exposes home function", () => {
            buildFixture();
            new Viewport();

            expect(typeof window.uhs.home).toBe("function");
        });

        it("exposes search function", () => {
            buildFixture();
            new Viewport();

            expect(typeof window.uhs.search).toBe("function");
        });

        it("back navigates to previous view", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            expect(getHeading()).toBe("The Cellar");

            window.uhs.back();
            expect(getHeading()).toBe("Shadowed Passage Atlas");
        });

        it("forward navigates to next view", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            window.uhs.back();
            expect(getHeading()).toBe("Shadowed Passage Atlas");

            window.uhs.forward();
            expect(getHeading()).toBe("The Cellar");
        });

        it("home navigates to home view", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            expect(getHeading()).toBe("The Cellar");

            window.uhs.home();
            expect(getHeading()).toBe("Shadowed Passage Atlas");
        });

        it("search displays results", () => {
            buildFixture();
            new Viewport();

            window.uhs.search("trapdoor");
            expect(getHeading()).toBe("Search: trapdoor");
        });
    });
});
