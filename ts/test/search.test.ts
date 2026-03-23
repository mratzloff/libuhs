import { beforeEach, describe, expect, it } from "vitest";

import search from "../src/search";

function createHintElement(
    id: string,
    title: string,
    hints: string[],
): HTMLDivElement {
    const element = document.createElement("div");
    element.setAttribute("data-type", "nesthint");
    element.setAttribute("data-id", id);

    const titleContainer = document.createElement("div");
    const titleSpan = document.createElement("span");
    titleSpan.classList.add("title");
    titleSpan.textContent = title;
    titleContainer.appendChild(titleSpan);
    element.appendChild(titleContainer);

    const list = document.createElement("ol");
    for (const hint of hints) {
        const item = document.createElement("li");
        const hintSpan = document.createElement("span");
        hintSpan.classList.add("hint");
        hintSpan.textContent = hint;
        item.appendChild(hintSpan);
        list.appendChild(item);
    }
    element.appendChild(list);

    return element;
}

function createTextElement(
    id: string,
    title: string,
    body: string,
): HTMLDivElement {
    const element = document.createElement("div");
    element.setAttribute("data-type", "text");
    element.setAttribute("data-id", id);

    const titleContainer = document.createElement("div");
    const titleSpan = document.createElement("span");
    titleSpan.classList.add("title");
    titleSpan.textContent = title;
    titleContainer.appendChild(titleSpan);
    element.appendChild(titleContainer);

    const paragraph = document.createElement("p");
    paragraph.textContent = body;
    element.appendChild(paragraph);

    return element;
}

describe("search", () => {
    let root: HTMLDivElement;

    beforeEach(() => {
        document.body.innerHTML = "";

        root = document.createElement("div");
        root.id = "root";
        document.body.appendChild(root);
    });

    it("returns empty when root is missing", () => {
        document.body.innerHTML = "";

        const results = search("anything");

        expect(results).toEqual([]);
    });

    it("returns empty for no matches", () => {
        root.appendChild(
            createHintElement("10", "Castle", ["Go north", "Find the key"]),
        );

        const results = search("dragon");

        expect(results).toEqual([]);
    });

    it("matches title text", () => {
        root.appendChild(
            createHintElement("10", "Castle Entrance", ["Go north"]),
        );

        const results = search("castle");

        expect(results).toHaveLength(1);
        expect(results[0].getAttribute("data-id")).toBe("10");
    });

    it("matches hint text", () => {
        root.appendChild(
            createHintElement("10", "Castle", [
                "Look under the rug",
                "Find the key",
            ]),
        );

        const results = search("key");

        expect(results).toHaveLength(1);
        expect(results[0].getAttribute("data-id")).toBe("10");
    });

    it("matches text element body", () => {
        root.appendChild(
            createTextElement(
                "10",
                "Introduction",
                "Welcome to the adventure game",
            ),
        );

        const results = search("adventure");

        expect(results).toHaveLength(1);
        expect(results[0].getAttribute("data-id")).toBe("10");
    });

    it("ranks title matches higher than hint matches", () => {
        root.appendChild(createHintElement("10", "Sword", ["Find the shield"]));
        root.appendChild(
            createHintElement("20", "Shield", [
                "The sword is nearby",
                "Look for the sword",
            ]),
        );

        const results = search("sword");

        expect(results).toHaveLength(2);
        expect(results[0].getAttribute("data-id")).toBe("10");
        expect(results[1].getAttribute("data-id")).toBe("20");
    });

    it("accumulates score from title and hint matches", () => {
        root.appendChild(
            createHintElement("10", "Find the key", [
                "The key is hidden",
                "Look for the key",
            ]),
        );

        const results = search("key");

        expect(results).toHaveLength(1);
        expect(results[0].getAttribute("data-id")).toBe("10");
    });

    it("skips the home element", () => {
        root.appendChild(createHintElement("1", "Home", ["Start here"]));

        const results = search("home");

        expect(results).toEqual([]);
    });

    it("skips document elements", () => {
        const element = document.createElement("div");
        element.setAttribute("data-type", "document");
        element.setAttribute("data-id", "99");

        const titleContainer = document.createElement("div");
        const titleSpan = document.createElement("span");
        titleSpan.classList.add("title");
        titleSpan.textContent = "Searchable Document";
        titleContainer.appendChild(titleSpan);
        element.appendChild(titleContainer);

        root.appendChild(element);

        const results = search("searchable");

        expect(results).toEqual([]);
    });

    it("skips elements with hidden ancestors", () => {
        const parent = document.createElement("div");
        parent.setAttribute("data-visibility", "none");

        parent.appendChild(
            createHintElement("10", "Hidden Section", ["Secret hint"]),
        );
        root.appendChild(parent);

        const results = search("hidden");

        expect(results).toEqual([]);
    });

    it("is case-insensitive", () => {
        root.appendChild(createHintElement("10", "CASTLE", ["Go NORTH"]));

        const titleResults = search("castle");
        const hintResults = search("north");

        expect(titleResults).toHaveLength(1);
        expect(hintResults).toHaveLength(1);
    });

    it("trims whitespace from query", () => {
        root.appendChild(createHintElement("10", "Castle", ["Go north"]));

        const results = search("  castle  ");

        expect(results).toHaveLength(1);
    });

    it("returns results sorted by score descending", () => {
        root.appendChild(
            createHintElement("10", "Forest", ["A dark forest path"]),
        );
        root.appendChild(
            createHintElement("20", "Dark Forest", [
                "Very dark here",
                "The dark path continues",
            ]),
        );

        const results = search("dark");

        expect(results).toHaveLength(2);
        expect(results[0].getAttribute("data-id")).toBe("20");
        expect(results[1].getAttribute("data-id")).toBe("10");
    });
});
