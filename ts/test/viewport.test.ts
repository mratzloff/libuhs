import { beforeEach, describe, expect, it, vi } from "vitest";

import Viewport from "../src/viewport";
import { buildFixture } from "./fixtures";
import {
    clickButton,
    clickFooterButton,
    clickLink,
    clickSearchReset,
    clickTitle,
    getBreadcrumbTexts,
    getClickableTitles,
    getFooterButtonText,
    getHeading,
    getHiddenHintCount,
    getLinks,
    getSearchField,
    getTitles,
    getViewport,
    getVisibleHints,
    hasBlankElement,
    typeSearch,
} from "./helpers";

// ── Tests ────────────────────────────────────────────────────────────

describe("Viewport", () => {
    beforeEach(() => {
        document.body.innerHTML = "";
        sessionStorage.clear();
        vi.stubGlobal("scrollTo", vi.fn());
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
        it("renders search results on input", () => {
            buildFixture();
            new Viewport();

            typeSearch("Cellar");

            expect(getHeading()).toBe("Search: Cellar");
        });

        it("does not search for whitespace-only input", () => {
            buildFixture();
            new Viewport();

            typeSearch("   ");

            expect(getHeading()).toBe("Shadowed Passage Atlas");
        });

        it("shows no results message when nothing matches", () => {
            buildFixture();
            new Viewport();

            typeSearch("nonexistent");

            expect(getViewport().textContent).toContain("No results found");
        });

        it("shows matching results as clickable links", () => {
            buildFixture();
            new Viewport();

            typeSearch("Tower");

            const results = getViewport().querySelectorAll(
                "ol.search-results li",
            );
            expect(results.length).toBeGreaterThanOrEqual(2);
        });

        it("navigates to hint when clicking search result", () => {
            buildFixture();
            new Viewport();

            typeSearch("Cellar");
            const result = getViewport().querySelector(
                "ol.search-results .title.clickable",
            ) as HTMLElement;
            result?.click();

            expect(getHeading()).toBe("The Cellar");
        });

        it("creates a single history entry across consecutive keystrokes", () => {
            buildFixture();
            new Viewport();

            typeSearch("C");
            typeSearch("Ce");
            typeSearch("Cel");
            typeSearch("Cellar");

            clickButton("back");
            expect(getHeading()).toBe("Shadowed Passage Atlas");
        });

        it("reset button navigates back from search", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            typeSearch("Tower");
            expect(getHeading()).toBe("Search: Tower");

            clickSearchReset();

            expect(getHeading()).toBe("The Cellar");
        });

        it("clears the search field on back from search", () => {
            buildFixture();
            new Viewport();

            typeSearch("Cellar");
            clickButton("back");

            expect(getSearchField().value).toBe("");
        });

        it("clears the search field on home from search", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            typeSearch("Tower");
            clickButton("home");

            expect(getSearchField().value).toBe("");
        });

        it("removes the search state from history on back", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            typeSearch("Tower");
            clickButton("back");

            expect(
                document.getElementById("forward")?.hasAttribute("disabled"),
            ).toBe(true);
        });

        it("removes the search state from history on home", () => {
            buildFixture();
            new Viewport();

            clickTitle("The Cellar");
            typeSearch("Tower");
            clickButton("home");

            clickButton("back");
            expect(getHeading()).toBe("The Cellar");
        });

        it("removes the search session including visited results on home", () => {
            buildFixture();
            new Viewport();

            typeSearch("Cellar");
            const result = getViewport().querySelector(
                "ol.search-results .title.clickable",
            ) as HTMLElement;
            result?.click();
            expect(getHeading()).toBe("The Cellar");

            clickButton("back");
            expect(getHeading()).toBe("Search: Cellar");

            clickButton("home");
            expect(getHeading()).toBe("Shadowed Passage Atlas");

            expect(
                document.getElementById("back")?.hasAttribute("disabled"),
            ).toBe(true);
        });

        it("allows forward within a search state", () => {
            buildFixture();
            new Viewport();

            typeSearch("Cellar");
            const result = getViewport().querySelector(
                "ol.search-results .title.clickable",
            ) as HTMLElement;
            result?.click();
            expect(getHeading()).toBe("The Cellar");

            clickButton("back");
            expect(getHeading()).toBe("Search: Cellar");

            clickButton("forward");
            expect(getHeading()).toBe("The Cellar");
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
});
