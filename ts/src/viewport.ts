import { HOME_ID } from "./constants";
import { History, HistoryState } from "./history";
import search from "./search";

const ViewType = {
    Hint: "hint",
    Search: "search",
};

class Viewport {
    private backButton: HTMLElement;
    private forwardButton: HTMLElement;
    private history: History;
    private tableToggle: HTMLInputElement;
    private viewport: HTMLElement;

    public static initOnReady() {
        document.addEventListener("DOMContentLoaded", () => new Viewport());
    }

    public constructor() {
        this.history = new History();

        // Create nav
        const nav = document.createElement("nav");
        nav.id = "nav";

        // Create back button
        this.backButton = document.createElement("button");
        this.backButton.id = "back";
        this.backButton.setAttribute("disabled", "true");
        this.backButton.addEventListener("click", () => this.back());
        nav.appendChild(this.backButton);

        // Create forward button
        this.forwardButton = document.createElement("button");
        this.forwardButton.id = "forward";
        this.forwardButton.setAttribute("disabled", "true");
        this.forwardButton.addEventListener("click", () => this.forward());
        nav.appendChild(this.forwardButton);

        // Create home button
        const homeButton = document.createElement("button");
        homeButton.id = "home";
        homeButton.addEventListener("click", () => this.home());
        nav.appendChild(homeButton);

        // Create spacer
        const spacer = document.createElement("div");
        spacer.classList.add("spacer");
        nav.appendChild(spacer);

        // Create search field
        const searchField = document.createElement("input");
        searchField.type = "search";
        searchField.placeholder = "Search";
        searchField.addEventListener("search", e => {
            const keywords = (e.target as HTMLInputElement).value.trim();
            if (keywords.length > 0) {
                this.go({ type: ViewType.Search, locator: keywords });
            }
        });
        nav.appendChild(searchField);

        // Create options menu button
        const optionsButton = document.createElement("button");
        optionsButton.id = "options";
        optionsButton.addEventListener("click", () => {
            optionsMenu.classList.toggle("open");
            this.viewport.classList.toggle("options-open");
        });
        nav.appendChild(optionsButton);

        // Append nav
        document.body.appendChild(nav);

        // Create options menu
        const optionsMenuWrapper = document.createElement("div");
        optionsMenuWrapper.id = "options-menu-wrapper";

        const optionsMenu = document.createElement("div");
        optionsMenu.id = "options-menu";

        const tableToggleLabel = document.createElement("label");
        tableToggleLabel.classList.add("toggle");

        const tableToggleText = document.createElement("span");
        tableToggleText.classList.add("toggle-label");
        tableToggleText.textContent = "HTML tables";
        tableToggleLabel.appendChild(tableToggleText);

        this.tableToggle = document.createElement("input");
        this.tableToggle.type = "checkbox";
        this.tableToggle.checked =
            sessionStorage.getItem("option.table.mode") === "html";
        this.tableToggle.addEventListener("change", () => {
            const mode = this.tableToggle.checked ? "html" : "text";
            sessionStorage.setItem("option.table.mode", mode);
            this.applyTableMode();
        });
        tableToggleLabel.appendChild(this.tableToggle);

        const toggleTrack = document.createElement("span");
        toggleTrack.classList.add("toggle-track");
        tableToggleLabel.appendChild(toggleTrack);

        optionsMenu.appendChild(tableToggleLabel);
        optionsMenuWrapper.appendChild(optionsMenu);
        document.body.appendChild(optionsMenuWrapper);

        // Create viewport
        this.viewport = document.createElement("div");
        this.viewport.id = "viewport";
        document.body.appendChild(this.viewport);

        // Handle history changes
        this.history.addEventListener("change", event => {
            this.view((event as CustomEvent).detail);
        });

        // Go to home view
        this.home();
    }

    private applyTableMode(): void {
        const showHtml = this.tableToggle.checked;

        this.viewport
            .querySelectorAll(".table-container > .option-html")
            .forEach(t => t.classList.toggle("hidden", !showHtml));
        this.viewport
            .querySelectorAll(".table-container > .option-text")
            .forEach(t => t.classList.toggle("hidden", showHtml));
    }

    private back(): void {
        this.history.back();
        this.refreshHistoryButtons();
        this.scrollTop();
    }

    private cloneEntryPoint(id: string): HTMLElement {
        const originalEntry = document.getElementById(id);
        if (!originalEntry) {
            throw new Error("could not find entry point");
        }

        return originalEntry.cloneNode(true) as HTMLElement;
    }

    private createFooter(items: HTMLElement[], elementId: string): void {
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
        const index = this.getHintCacheIndex(elementId);
        if (items.length == 1 || index + 1 == items.length) {
            button.textContent = "Back";
        } else {
            button.textContent = "Show next hint";
        }
        button.addEventListener("click", () =>
            this.onButtonClick(items, elementId),
        );
        footer.appendChild(button);

        document.body.appendChild(footer);
    }

    private createLinkClickHandlers(element: HTMLElement): void {
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
                link.addEventListener("click", () => this.go(state), {
                    capture: false,
                });
                link.classList.add("clickable");
            }

            link.removeAttribute("href");
        });
    }

    private createOverlayClickHandler(element: HTMLElement): void {
        const overlays = element.querySelectorAll("area[data-overlay]");
        overlays.forEach(overlay => {
            const targetId = overlay.getAttribute("data-overlay");
            if (!targetId) {
                throw new Error("could not find overlay target ID");
            }

            overlay.addEventListener(
                "click",
                () => this.showOverlay(targetId),
                { capture: false },
            );
            overlay.classList.add("clickable");
        });
    }

    private createTitleClickHandlers(element: HTMLElement): void {
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
            title.addEventListener("click", () => this.go(state), {
                capture: false,
            });
            title.classList.add("clickable");
        });
    }

    private findListItemChildren(
        element: HTMLElement,
    ): NodeListOf<HTMLElement> {
        return element.querySelectorAll(
            ":scope > ol > li",
        ) as NodeListOf<HTMLElement>;
    }

    private forward(): void {
        this.history.forward();
        this.refreshHistoryButtons();
        this.scrollTop();
    }

    private getHintCacheIndex(elementId: string): number {
        let index = 0;

        const key = this.getHintCacheKey(elementId);
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

    private getHintCacheKey(elementId: string): string {
        return `hint.${elementId}`;
    }

    private go(state: HistoryState): void {
        this.history.pushState(state);
        this.refreshHistoryButtons();
        this.view(state);
        this.scrollTop();
    }

    private hide(): void {
        this.viewport.style.display = "none";
    }

    private home(): void {
        this.go({ type: ViewType.Hint, locator: HOME_ID });
    }

    private onButtonClick(items: HTMLElement[], elementId: string): void {
        for (let i = 0; i < items.length; ++i) {
            const item = items[i];
            if (!item.hidden) {
                continue;
            }

            item.removeAttribute("hidden");
            this.updateHintProgress((i + 1) / items.length);
            this.setHintCacheIndex(elementId, i);
            this.scrollBottom();

            if (i + 1 == items.length) {
                this.setButtonText("Back");
            }

            return;
        }

        this.removeFooter();
        this.back();
    }

    private processHintNode(element: HTMLElement): void {
        const type = element.getAttribute("data-type");
        if (type != "hint" && type != "nesthint") {
            return;
        }

        const index = this.getHintCacheIndex(element.id);
        const items = Array.from(this.findListItemChildren(element));
        for (let i = index + 1; i < items.length; ++i) {
            items[i].hidden = true;
        }

        this.createFooter(items, element.id);
        this.updateHintProgress((index + 1) / items.length);
    }

    private processLeafNode(element: HTMLElement): void {
        this.replaceIdsWithDataAttributes(element);
        this.updateImageMapAnchor(element);
        this.createOverlayClickHandler(element);
    }

    private processListNode(element: HTMLElement): void {
        if (!element.firstChild) {
            throw new Error("empty list item");
        }

        this.removeUnnecessaryElements(element);
        this.replaceIdsWithDataAttributes(element);
        this.createTitleClickHandlers(element);
    }

    private refreshHistoryButtons() {
        if (this.history.hasPrevious()) {
            this.backButton.removeAttribute("disabled");
        } else {
            this.backButton.setAttribute("disabled", "true");
        }

        if (this.history.hasNext()) {
            this.forwardButton.removeAttribute("disabled");
        } else {
            this.forwardButton.setAttribute("disabled", "true");
        }
    }

    private removeFooter(): void {
        const footer = document.getElementById("footer");
        footer?.remove();
    }

    private removeUnnecessaryElements(element: HTMLElement): void {
        const container = element.firstElementChild;
        if (!container) {
            return;
        }

        for (let i = container.children.length - 1; i >= 1; --i) {
            const child = container.children[i];
            if (child.querySelector(":scope > div > span.title")) {
                for (let j = child.children.length - 1; j >= 1; --j) {
                    child.removeChild(child.children[j]);
                }
            } else {
                container.removeChild(child);
            }
        }
    }

    private renderElement(element: HTMLElement): void {
        this.viewport.appendChild(element);
        this.applyTableMode();
    }

    private findBreadcrumbs(element: HTMLElement): string[] {
        let ancestors: string[] = [];
        let ancestor: HTMLElement | null | undefined = element;

        while (ancestor?.parentElement) {
            ancestor = ancestor.parentElement;

            const id = ancestor.getAttribute("data-id");
            if (id == HOME_ID) {
                break;
            }
            if (id) {
                const title = this.findNodeTitle(ancestor);
                if (title) {
                    ancestors.push(title);
                }
            }
        }

        return ancestors.reverse();
    }

    private findNodeTitle(element: HTMLElement): string | null {
        const titleNode = element.querySelector(":scope > div > span.title");
        return titleNode?.textContent || null;
    }

    private renderSearchResult(result: HTMLElement): HTMLLIElement {
        const title = this.findNodeTitle(result);
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
        link.addEventListener("click", () => this.go(state), {
            capture: false,
        });
        link.classList.add("title", "clickable");
        link.textContent = title;
        linkContainer.appendChild(link);
        item.appendChild(linkContainer);

        // Render breadcrumbs
        const breadcrumbs = this.renderBreadcrumbs(result);
        if (breadcrumbs) {
            item.appendChild(breadcrumbs);
        }

        return item;
    }

    private renderBreadcrumbs(element: HTMLElement): HTMLUListElement | null {
        const breadcrumbs = this.findBreadcrumbs(element);
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

    private replaceIdsWithDataAttributes(element: HTMLElement): void {
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

    private replaceTitleWithHeading(element: HTMLElement): void {
        const title = element.querySelector(".title");
        if (!title || title.textContent === null) {
            throw new Error("could not find title");
        }

        const heading = document.createElement("h1");
        heading.appendChild(document.createTextNode(title.textContent));
        title!.parentNode?.replaceChild(heading, title);
    }

    private reset(): void {
        while (this.viewport.lastChild) {
            this.viewport.removeChild(this.viewport.lastChild);
        }
        this.removeFooter();
    }

    private search(keywords: string): void {
        if (keywords.trim().length == 0) {
            return;
        }

        this.reset();
        const results = search(keywords);
        const heading = document.createElement("h1");
        heading.appendChild(document.createTextNode(`Search: ${keywords}`));
        this.viewport.appendChild(heading);

        if (results.length > 0) {
            const list = document.createElement("ol");
            list.classList.add("search-results");
            results.forEach(result =>
                list.appendChild(this.renderSearchResult(result)),
            );
            this.viewport.appendChild(list);
        } else {
            const text = document.createTextNode("No results found.");
            this.viewport.appendChild(text);
        }
    }

    private setButtonText(text: string): void {
        const button = document.getElementById("button");
        if (!button) {
            throw new Error("could not find button");
        }
        button.textContent = text;
    }

    private setHintCacheIndex(elementId: string, value: number): void {
        const key = this.getHintCacheKey(elementId);
        sessionStorage.setItem(key, value.toString());
    }

    private showOverlay(id: string): void {
        const overlay = this.viewport.querySelector(`[data-id="${id}"]`);
        if (!overlay) {
            throw new Error("could not find overlay");
        }
        overlay.removeAttribute("hidden");
    }

    private show(): void {
        this.viewport.style.display = "block";
    }

    private scrollBottom(): void {
        scrollTo(0, document.body.scrollHeight);
    }

    private scrollTop(): void {
        scrollTo(0, 0);
    }

    private updateHintProgress(value: number): void {
        const progress = document.getElementById("progress");
        progress?.style.setProperty("width", `${value * 100}%`);
    }

    private updateImageMapAnchor(element: HTMLElement): void {
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

    private view(state: HistoryState): void {
        this.hide();
        this.reset();

        switch (state.type) {
            case ViewType.Search:
                this.search(state.locator);
                break;
            case ViewType.Hint:
                const entry = this.cloneEntryPoint(state.locator);

                const element = document.getElementById(state.locator)!;
                const headingContainer = entry.querySelector(":scope > div");
                if (headingContainer) {
                    const breadcrumbs = this.renderBreadcrumbs(element);
                    if (breadcrumbs) {
                        headingContainer.prepend(breadcrumbs);
                    }
                }
                this.replaceTitleWithHeading(entry);

                const items = this.findListItemChildren(entry);

                if (items.length > 0) {
                    items.forEach(item => this.processListNode(item));
                } else {
                    this.processLeafNode(entry);
                }

                this.createLinkClickHandlers(entry);
                this.processHintNode(entry);
                this.renderElement(entry);
                break;
        }

        this.scrollTop();
        this.show();
    }
}

export default Viewport;
