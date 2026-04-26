import { HOME_ID } from "./constants";
import { History, HistoryChangeCallback, HistoryState } from "./history";
import search from "./search";

const ViewType = {
    Hint: "hint",
    Search: "search",
};

class Viewport {
    public constructor() {
        this.#history = new History();
        this.#history.addEventListener("change", event => {
            this.#view((event as CustomEvent).detail);
        });
        this.#history.onChange = (state, hasPrevious, hasNext) => {
            this.onHistoryChange?.(state, hasPrevious, hasNext);
        };

        this.#createNav();
        this.#createOptionsMenu();
        this.#createViewport();
        this.home();
    }

    public onHistoryChange: HistoryChangeCallback = null;

    public get state(): HistoryState | null {
        return this.#history.state;
    }

    public back(): void {
        if (this.#isSearchView()) {
            this.#resetSearch();
            this.#truncateSearchHistory();
        }

        this.#history.back();
        this.#refreshHistoryButtons();
        this.#scrollTop();
    }

    public forward(): void {
        this.#history.forward();
        this.#refreshHistoryButtons();
        this.#scrollTop();
    }

    public hasNext(): boolean {
        return this.#history.hasNext();
    }

    public hasPrevious(): boolean {
        return this.#history.hasPrevious();
    }

    public home(): void {
        this.#resetSearch();
        if (this.#isSearchView()) {
            this.#truncateSearchHistory();
        }
        this.#go({ type: ViewType.Hint, locator: HOME_ID });
    }

    public search(keywords: string): void {
        keywords = keywords.trim();
        if (keywords.length == 0) {
            return;
        }

        const state = { type: ViewType.Search, locator: keywords };
        if (this.#isSearchView()) {
            this.#go(state, { replaceState: true });
        } else {
            this.#go(state);
        }
    }

    #backButton!: HTMLElement;
    #forwardButton!: HTMLElement;
    #history: History;
    #searchField!: HTMLInputElement;
    #tableToggle!: HTMLInputElement;
    #viewport!: HTMLElement;

    #applyTableMode(): void {
        const showHtml = this.#tableToggle.checked;

        this.#viewport
            .querySelectorAll(".table-container > .option-html")
            .forEach(t => t.classList.toggle("hidden", !showHtml));
        this.#viewport
            .querySelectorAll(".table-container > .option-text")
            .forEach(t => t.classList.toggle("hidden", showHtml));
    }

    #cloneEntryPoint(id: string): HTMLElement {
        const originalEntry = document.getElementById(id);
        if (!originalEntry) {
            throw new Error("could not find entry point");
        }

        return originalEntry.cloneNode(true) as HTMLElement;
    }

    #createFooter(items: HTMLElement[], elementId: string): void {
        const footer = document.createElement("footer");
        footer.id = "footer";

        const progressContainer = document.createElement("div");
        progressContainer.id = "progress-container";
        const progress = document.createElement("div");
        progress.id = "progress";
        progressContainer.appendChild(progress);
        footer.appendChild(progressContainer);

        const button = document.createElement("button");
        button.id = "button";
        const index = this.#getHintCacheIndex(elementId);
        if (items.length == 1 || index + 1 == items.length) {
            button.textContent = "Back";
        } else {
            button.textContent = "Show next hint";
        }
        button.addEventListener("click", () =>
            this.#onButtonClick(items, elementId),
        );
        footer.appendChild(button);

        document.body.appendChild(footer);
    }

    #createLinkClickHandlers(element: HTMLElement): void {
        const links = element.querySelectorAll(
            "a[href]:not(.hyperlink), area[href]",
        );
        links.forEach(link => {
            const targetId = link.getAttribute("data-target");
            if (!targetId) {
                throw new Error("could not find link target ID");
            }

            let clickable = true;
            if (link.nodeName == "AREA") {
                const target = document.getElementById(targetId);
                clickable = target?.nodeName == "DIV";
            }

            if (clickable) {
                const state = { type: ViewType.Hint, locator: targetId };
                link.addEventListener("click", () => this.#go(state), {
                    capture: false,
                });
                link.classList.add("clickable");
            }

            link.removeAttribute("href");
        });
    }

    #createNav(): void {
        const nav = document.createElement("nav");
        nav.id = "nav";

        this.#backButton = document.createElement("button");
        this.#backButton.id = "back";
        this.#backButton.setAttribute("disabled", "true");
        this.#backButton.addEventListener("click", () => this.back());
        nav.appendChild(this.#backButton);

        this.#forwardButton = document.createElement("button");
        this.#forwardButton.id = "forward";
        this.#forwardButton.setAttribute("disabled", "true");
        this.#forwardButton.addEventListener("click", () => this.forward());
        nav.appendChild(this.#forwardButton);

        const homeButton = document.createElement("button");
        homeButton.id = "home";
        homeButton.addEventListener("click", () => this.home());
        nav.appendChild(homeButton);

        const spacer = document.createElement("div");
        spacer.classList.add("spacer");
        nav.appendChild(spacer);

        const searchContainer = document.createElement("search");
        this.#searchField = document.createElement("input");
        this.#searchField.minLength = 2;
        this.#searchField.type = "search";
        this.#searchField.placeholder = "Search";
        this.#searchField.addEventListener("input", e => {
            if (!(e instanceof InputEvent)) {
                // Reset button
                this.back();
                return;
            }

            const keywords = (e.target as HTMLInputElement).value.trim();
            if (keywords.length > 0) {
                this.search(keywords);
            }
        });
        searchContainer.appendChild(this.#searchField);
        nav.appendChild(searchContainer);

        const optionsButton = document.createElement("button");
        optionsButton.id = "options";
        optionsButton.addEventListener("click", () => {
            const menu = document.getElementById("options-menu");
            menu?.classList.toggle("open");
            this.#viewport.classList.toggle("options-open");
        });
        nav.appendChild(optionsButton);

        document.body.appendChild(nav);
    }

    #createOptionsMenu(): void {
        const wrapper = document.createElement("div");
        wrapper.id = "options-menu-wrapper";

        const menu = document.createElement("div");
        menu.id = "options-menu";

        const label = document.createElement("label");
        label.classList.add("toggle");

        const text = document.createElement("span");
        text.classList.add("toggle-label");
        text.textContent = "HTML tables";
        label.appendChild(text);

        this.#tableToggle = document.createElement("input");
        this.#tableToggle.type = "checkbox";
        this.#tableToggle.checked =
            sessionStorage.getItem("option.table.mode") === "html";
        this.#tableToggle.addEventListener("change", () => {
            const mode = this.#tableToggle.checked ? "html" : "text";
            sessionStorage.setItem("option.table.mode", mode);
            this.#applyTableMode();
        });
        label.appendChild(this.#tableToggle);

        const track = document.createElement("span");
        track.classList.add("toggle-track");
        label.appendChild(track);

        menu.appendChild(label);
        wrapper.appendChild(menu);
        document.body.appendChild(wrapper);
    }

    #createOverlayClickHandler(element: HTMLElement): void {
        const overlays = element.querySelectorAll("area[data-overlay]");
        overlays.forEach(overlay => {
            const targetId = overlay.getAttribute("data-overlay");
            if (!targetId) {
                throw new Error("could not find overlay target ID");
            }

            overlay.addEventListener(
                "click",
                () => this.#showOverlay(targetId),
                { capture: false },
            );
            overlay.classList.add("clickable");
        });
    }

    #createTitleClickHandlers(element: HTMLElement): void {
        const titles = element.querySelectorAll(".title");
        titles.forEach(title => {
            const element = title.parentElement?.parentElement;
            if (!element) {
                throw new Error("could not find title element");
            }

            const state = {
                type: ViewType.Hint,
                locator: element.getAttribute("data-id")!,
            };
            title.addEventListener("click", () => this.#go(state), {
                capture: false,
            });
            title.classList.add("clickable");
        });
    }

    #createViewport(): void {
        this.#viewport = document.createElement("div");
        this.#viewport.id = "viewport";
        document.body.appendChild(this.#viewport);
    }

    #findBreadcrumbs(element: HTMLElement): string[] {
        const ancestors: string[] = [];
        let ancestor: HTMLElement | null | undefined = element;

        while (ancestor?.parentElement) {
            ancestor = ancestor.parentElement;

            const id = ancestor.getAttribute("data-id");
            if (id == HOME_ID) {
                break;
            }
            if (id) {
                const title = this.#findNodeTitle(ancestor);
                if (title) {
                    ancestors.push(title);
                }
            }
        }

        return ancestors.reverse();
    }

    #findListItemChildren(element: HTMLElement): NodeListOf<HTMLElement> {
        return element.querySelectorAll(
            ":scope > ol > li",
        ) as NodeListOf<HTMLElement>;
    }

    #findNodeTitle(element: HTMLElement): string | null {
        const titleNode = element.querySelector(":scope > div > span.title");
        return titleNode?.textContent || null;
    }

    #getHintCacheIndex(elementId: string): number {
        let index = 0;

        const key = this.#getHintCacheKey(elementId);
        const cacheValue = sessionStorage.getItem(key);

        if (cacheValue) {
            const parsed = parseInt(cacheValue, 10);
            if (Number.isNaN(parsed)) {
                sessionStorage.removeItem(key);
            }
            index = parsed;
        }

        return index;
    }

    #getHintCacheKey(elementId: string): string {
        return `hint.${elementId}`;
    }

    #go(state: HistoryState, { replaceState = false } = {}): void {
        if (replaceState) {
            this.#history.replaceState(state);
        } else {
            this.#history.pushState(state);
        }

        this.#refreshHistoryButtons();
        this.#view(state);
        this.#scrollTop();
    }

    #hide(): void {
        this.#viewport.style.display = "none";
    }

    #isSearchView(): boolean {
        return this.#history.state?.type == ViewType.Search;
    }

    #onButtonClick(items: HTMLElement[], elementId: string): void {
        for (let i = 0; i < items.length; ++i) {
            const item = items[i];
            if (!item.hidden) {
                continue;
            }

            item.removeAttribute("hidden");
            this.#updateHintProgress((i + 1) / items.length);
            this.#setHintCacheIndex(elementId, i);
            this.#scrollBottom();

            if (i + 1 == items.length) {
                this.#setButtonText("Back");
            }

            return;
        }

        this.#removeFooter();
        this.back();
    }

    #processHintNode(element: HTMLElement): void {
        const type = element.getAttribute("data-type");
        if (type != "hint" && type != "nesthint") {
            return;
        }

        const index = this.#getHintCacheIndex(element.id);
        const items = Array.from(this.#findListItemChildren(element));
        for (let i = index + 1; i < items.length; ++i) {
            items[i].hidden = true;
        }

        this.#createFooter(items, element.id);
        this.#updateHintProgress((index + 1) / items.length);
    }

    #processLeafNode(element: HTMLElement): void {
        this.#replaceIdsWithDataAttributes(element);
        this.#updateImageMapAnchor(element);
        this.#createOverlayClickHandler(element);
    }

    #processListNode(element: HTMLElement): void {
        if (!element.firstChild) {
            throw new Error("empty list item");
        }

        this.#removeUnnecessaryElements(element);
        this.#replaceIdsWithDataAttributes(element);
        this.#createTitleClickHandlers(element);
    }

    #refreshHistoryButtons(): void {
        if (this.#history.hasPrevious()) {
            this.#backButton.removeAttribute("disabled");
        } else {
            this.#backButton.setAttribute("disabled", "true");
        }

        if (this.#history.hasNext()) {
            this.#forwardButton.removeAttribute("disabled");
        } else {
            this.#forwardButton.setAttribute("disabled", "true");
        }
    }

    #removeFooter(): void {
        const footer = document.getElementById("footer");
        footer?.remove();
    }

    #removeUnnecessaryElements(element: HTMLElement): void {
        const container = element.firstElementChild;
        if (container && container.tagName === "DIV") {
            const inlineTags = new Set(["SPAN", "A"]);
            for (let i = container.children.length - 1; i >= 0; --i) {
                const child = container.children[i];
                if (inlineTags.has(child.tagName)) {
                    continue;
                }
                if (child.querySelector(":scope > div > span.title")) {
                    for (let j = child.children.length - 1; j >= 1; --j) {
                        child.removeChild(child.children[j]);
                    }
                } else if (i >= 1) {
                    container.removeChild(child);
                }
            }
        }
    }

    #renderBreadcrumbs(element: HTMLElement): HTMLUListElement | null {
        const breadcrumbs = this.#findBreadcrumbs(element);
        if (breadcrumbs.length == 0) {
            return null;
        }

        const list = document.createElement("ul");
        list.classList.add("breadcrumbs");
        breadcrumbs.forEach(breadcrumb => {
            const item = document.createElement("li");
            item.appendChild(document.createTextNode(breadcrumb));
            list.appendChild(item);
        });

        return list;
    }

    #renderElement(element: HTMLElement): void {
        this.#viewport.appendChild(element);
        this.#applyTableMode();
    }

    #renderSearch(keywords: string): void {
        this.#resetViewport();
        const results = search(keywords);
        const heading = document.createElement("h1");
        heading.appendChild(document.createTextNode(`Search: ${keywords}`));
        this.#viewport.appendChild(heading);

        if (results.length > 0) {
            const list = document.createElement("ol");
            list.classList.add("search-results");
            results.forEach(result =>
                list.appendChild(this.#renderSearchResult(result)),
            );
            this.#viewport.appendChild(list);
        } else {
            const text = document.createTextNode("No results found.");
            this.#viewport.appendChild(text);
        }
    }

    #renderSearchResult(result: HTMLElement): HTMLLIElement {
        const title = this.#findNodeTitle(result);
        if (!title) {
            throw new Error("could not find search result title");
        }

        const id = result.getAttribute("data-id");
        if (!id) {
            throw new Error("could not find search result ID");
        }

        // Render search result link
        const item = document.createElement("li");
        const linkContainer = document.createElement("div");
        const link = document.createElement("span");
        const state = { type: ViewType.Hint, locator: id };
        link.addEventListener("click", () => this.#go(state), {
            capture: false,
        });
        link.classList.add("title", "clickable");
        link.textContent = title;
        linkContainer.appendChild(link);
        item.appendChild(linkContainer);

        // Render breadcrumbs
        const breadcrumbs = this.#renderBreadcrumbs(result);
        if (breadcrumbs) {
            item.appendChild(breadcrumbs);
        }

        return item;
    }

    #replaceIdsWithDataAttributes(element: HTMLElement): void {
        if (element.id) {
            element.setAttribute("data-id", element.id);
            element.removeAttribute("id");
        }

        const children = element.querySelectorAll("[id]");
        children.forEach(child => {
            if (child.id) {
                child.setAttribute("data-id", child.id);
                child.removeAttribute("id");
            }
        });
    }

    #replaceTitleWithHeading(element: HTMLElement): void {
        const title = element.querySelector(".title");
        if (!title || title.textContent === null) {
            throw new Error("could not find title");
        }

        const heading = document.createElement("h1");
        heading.appendChild(document.createTextNode(title.textContent));
        title!.parentNode?.replaceChild(heading, title);
    }

    #resetSearch(): void {
        this.#searchField.value = "";
    }

    #resetViewport(): void {
        while (this.#viewport.lastChild) {
            this.#viewport.removeChild(this.#viewport.lastChild);
        }
        this.#removeFooter();
    }

    #scrollBottom(): void {
        scrollTo(0, document.body.scrollHeight);
    }

    #scrollTop(): void {
        scrollTo(0, 0);
    }

    #setButtonText(text: string): void {
        const button = document.getElementById("button");
        if (!button) {
            throw new Error("could not find button");
        }
        button.textContent = text;
    }

    #setHintCacheIndex(elementId: string, value: number): void {
        const key = this.#getHintCacheKey(elementId);
        sessionStorage.setItem(key, value.toString());
    }

    #show(): void {
        this.#viewport.style.display = "block";
    }

    #showOverlay(id: string): void {
        const overlay = this.#viewport.querySelector(`[data-id="${id}"]`);
        if (!overlay) {
            throw new Error("could not find overlay");
        }
        overlay.removeAttribute("hidden");
    }

    #truncateSearchHistory(): void {
        this.#history.truncate();
    }

    #updateHintProgress(value: number): void {
        const progress = document.getElementById("progress");
        progress?.style.setProperty("width", `${value * 100}%`);
    }

    #updateImageMapAnchor(element: HTMLElement): void {
        const map = element.querySelector("map");
        if (!map) {
            return;
        }

        const name = map.getAttribute("name");
        if (!name) {
            throw new Error("could not find map name");
        }
        map.setAttribute("name", `${name}_`);

        const hyperpng = element.querySelector(`img.hyperpng-background`);
        if (hyperpng) {
            hyperpng.setAttribute("usemap", `#${name}_`);
        }
    }

    #view(state: HistoryState): void {
        this.#hide();
        this.#resetViewport();

        switch (state.type) {
            case ViewType.Hint: {
                const entry = this.#cloneEntryPoint(state.locator);

                const element = document.getElementById(state.locator)!;
                const headingContainer = entry.querySelector(":scope > div");
                if (headingContainer) {
                    const breadcrumbs = this.#renderBreadcrumbs(element);
                    if (breadcrumbs) {
                        headingContainer.prepend(breadcrumbs);
                    }
                }
                this.#replaceTitleWithHeading(entry);

                const items = this.#findListItemChildren(entry);

                if (items.length > 0) {
                    items.forEach(item => this.#processListNode(item));
                } else {
                    this.#processLeafNode(entry);
                }

                this.#createLinkClickHandlers(entry);
                this.#processHintNode(entry);
                this.#renderElement(entry);
                break;
            }
            case ViewType.Search:
                this.#renderSearch(state.locator);
                break;
        }

        this.#scrollTop();
        this.#show();
    }
}

export default Viewport;
