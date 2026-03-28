import { beforeEach, describe, expect, it, vi } from "vitest";

import { publishAPI } from "../src/uhs";
import Viewport from "../src/viewport";
import { buildFixture } from "./fixtures";
import { clickTitle, getHeading } from "./helpers";

// ── Helpers ──────────────────────────────────────────────────────────

function createViewportWithAPI(): Viewport {
    const viewport = new Viewport();
    publishAPI(viewport);

    return viewport;
}

// ── Tests ────────────────────────────────────────────────────────────

describe("window.uhs", () => {
    beforeEach(() => {
        document.body.innerHTML = "";
        document.title = "";
        sessionStorage.clear();
        vi.stubGlobal("scrollTo", vi.fn());
    });

    it("exposes back function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(window.uhs).toBeDefined();
        expect(typeof window.uhs.back).toBe("function");
    });

    it("exposes forward function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(typeof window.uhs.forward).toBe("function");
    });

    it("exposes hasNext function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(typeof window.uhs.hasNext).toBe("function");
    });

    it("exposes hasPrevious function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(typeof window.uhs.hasPrevious).toBe("function");
    });

    it("exposes home function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(typeof window.uhs.home).toBe("function");
    });

    it("exposes search function", () => {
        buildFixture();
        createViewportWithAPI();

        expect(typeof window.uhs.search).toBe("function");
    });

    it("back navigates to previous view", () => {
        buildFixture();
        createViewportWithAPI();

        clickTitle("The Cellar");
        expect(getHeading()).toBe("The Cellar");

        window.uhs.back();
        expect(getHeading()).toBe("Shadowed Passage Atlas");
    });

    it("forward navigates to next view", () => {
        buildFixture();
        createViewportWithAPI();

        clickTitle("The Cellar");
        window.uhs.back();
        expect(getHeading()).toBe("Shadowed Passage Atlas");

        window.uhs.forward();
        expect(getHeading()).toBe("The Cellar");
    });

    it("home navigates to home view", () => {
        buildFixture();
        createViewportWithAPI();

        clickTitle("The Cellar");
        expect(getHeading()).toBe("The Cellar");

        window.uhs.home();
        expect(getHeading()).toBe("Shadowed Passage Atlas");
    });

    it("hasNext returns false at initial state", () => {
        buildFixture();
        createViewportWithAPI();

        expect(window.uhs.hasNext()).toBe(false);
    });

    it("hasPrevious returns false at initial state", () => {
        buildFixture();
        createViewportWithAPI();

        expect(window.uhs.hasPrevious()).toBe(false);
    });

    it("hasPrevious returns true after navigating", () => {
        buildFixture();
        createViewportWithAPI();

        clickTitle("The Cellar");
        expect(window.uhs.hasPrevious()).toBe(true);
    });

    it("hasNext returns true after navigating back", () => {
        buildFixture();
        createViewportWithAPI();

        clickTitle("The Cellar");
        window.uhs.back();
        expect(window.uhs.hasNext()).toBe(true);
    });

    it("onHistoryChange is called on navigation", () => {
        buildFixture();
        createViewportWithAPI();

        let callCount = 0;
        let lastHasPrevious = false;
        let lastHasNext = false;
        window.uhs.onHistoryChange = (hasPrevious, hasNext) => {
            callCount++;
            lastHasPrevious = hasPrevious;
            lastHasNext = hasNext;
        };

        clickTitle("The Cellar");
        expect(callCount).toBeGreaterThan(0);
        expect(lastHasPrevious).toBe(true);
        expect(lastHasNext).toBe(false);
    });

    it("onHistoryChange receives correct state after back", () => {
        buildFixture();
        createViewportWithAPI();

        let lastHasPrevious = false;
        let lastHasNext = false;
        window.uhs.onHistoryChange = (hasPrevious, hasNext) => {
            lastHasPrevious = hasPrevious;
            lastHasNext = hasNext;
        };

        clickTitle("The Cellar");
        window.uhs.back();
        expect(lastHasPrevious).toBe(false);
        expect(lastHasNext).toBe(true);
    });

    it("onHistoryChange is null by default", () => {
        buildFixture();
        createViewportWithAPI();

        expect(window.uhs.onHistoryChange).toBeNull();
    });

    it("search displays results", () => {
        buildFixture();
        createViewportWithAPI();

        window.uhs.search("trapdoor");
        expect(getHeading()).toBe("Search: trapdoor");
    });

    it("title matches the document root title", () => {
        buildFixture();
        createViewportWithAPI();

        expect(window.uhs.title).toBe("Shadowed Passage Atlas");
    });

    it("title sets document.title", () => {
        buildFixture();
        createViewportWithAPI();

        expect(document.title).toBe("Shadowed Passage Atlas");
    });
});
