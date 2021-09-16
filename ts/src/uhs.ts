class Viewport {
    private viewport: HTMLElement;

    public static initOnReady() {
        document.addEventListener("DOMContentLoaded", () => new Viewport);
    }

    public constructor() {
        this.viewport = document.createElement("div");
        this.viewport.id = "viewport";
        document.body.appendChild(this.viewport);

        const rootDocument = document.querySelector(`#root > section[data-type="document"]`);
        const version = rootDocument?.getAttribute("data-version");
        const id = (version === "88a") ? "5" : "1";

        this.view(id, this.viewport);
    }

    private showOverlay(id: string, viewport: HTMLElement): void {
        const overlay = viewport.querySelector(`[data-id="${id}"]`);
        if (!overlay) {
            throw new Error("could not find overlay");
        }
        overlay.removeAttribute("hidden");
    }

    private cloneEntryPoint(id: string): HTMLElement {
        const originalEntry = document.getElementById(id);
        if (!originalEntry) {
            throw new Error("could not find entry point");
        }

        return originalEntry.cloneNode(true) as HTMLElement;
    }

    private createLinkClickHandlers(container: HTMLElement): void {
        const links = container.querySelectorAll("a[href], area[href]");
        links.forEach(link => {
            const targetId = link.getAttribute("data-target");
            if (!targetId) {
                throw new Error("could not find link target ID");
            }

            let clickable = true;
            if (link.nodeName == "AREA") {
                const target = document.getElementById(targetId);
                clickable = (target?.nodeName == "DIV");
            }

            if (clickable) {
                link.addEventListener("click", () => this.view(targetId, this.viewport), {capture: false});
                link.classList.add("clickable");
            }
            link.removeAttribute("href");
        });
    }

    private createOverlayClickHandler(container: HTMLElement): void {
        const overlays = container.querySelectorAll("area[data-overlay]");
        overlays.forEach(overlay => {
            const targetId = overlay.getAttribute("data-overlay");
            if (!targetId) {
                throw new Error("could not find overlay target ID");
            }
            overlay.addEventListener("click", () => this.showOverlay(targetId, this.viewport), {capture: false});
            overlay.classList.add("clickable");
        });
    }

    private createTitleClickHandlers(container: HTMLElement): void {
        const titles = container.querySelectorAll(".title");
        titles.forEach(title => {
            const container = title.parentElement?.parentElement;
            if (!container) {
                throw new Error("could not find title container");
            }

            const id = container.getAttribute("data-id")!;
            title.addEventListener("click", () => this.view(id, this.viewport), {capture: false});
            title.classList.add("clickable");
        });
    }

    private findListItemChildren(container: HTMLElement): NodeListOf<HTMLElement> {
        return container.querySelectorAll(":scope > ol > li") as NodeListOf<HTMLElement>;
    }

    private processBranchNode(container: HTMLElement): void {
        if (!container.firstChild) {
            throw new Error("empty list item");
        }

        this.removeUnnecessaryElements(container);
        this.replaceIdsWithDataAttributes(container);
        this.createTitleClickHandlers(container);
    }

    private processLeafNode(container: HTMLElement): void {
        this.replaceIdsWithDataAttributes(container);
        this.updateImageMapAnchor(container);
        this.createOverlayClickHandler(container);
    }

    private removeUnnecessaryElements(container: HTMLElement): void {
        container.querySelector("p:not(.title)")?.remove();
        container.querySelector(".hyperpng-container")?.remove();
        container.querySelector(".sound")?.remove();
        const lists = container.querySelectorAll("ol");
        lists.forEach(list => list.remove());
    }

    private replaceIdsWithDataAttributes(container: HTMLElement): void {
        const elements = container.querySelectorAll("[id]");
        elements.forEach(element => {
            element.setAttribute("data-id", element.id);
            element.removeAttribute("id");
        });
    }

    private replaceTitleWithHeading(container: HTMLElement): void {
        const title = container.querySelector(".title");
        if (!title || title.textContent === null) {
            throw new Error("could not find title");
        }
        const heading = document.createElement("h1");
        heading.appendChild(document.createTextNode(title.textContent));
        title!.parentNode?.replaceChild(heading, title);
    }

    private render(container: HTMLElement): void {
        this.viewport.appendChild(container);
    }

    private reset(): void {
        while (this.viewport.lastChild) {
            this.viewport.removeChild(this.viewport.lastChild);
        }
    }

    private updateImageMapAnchor(container: HTMLElement): void {
        const map = container.querySelector("map");
        if (map) {
            const name = map.getAttribute("name");
            if (!name) {
                throw new Error("could not find map name");
            }
            map.setAttribute("name", `${name}_`);

            const hyperpng = container.querySelector(`img.hyperpng-background`);
            if (hyperpng) {
                hyperpng.setAttribute("usemap", `#${name}_`);
            }
        }
    }

    private view(id: string, viewport: HTMLElement): void {
        this.reset();
        const entry = this.cloneEntryPoint(id);
        this.replaceTitleWithHeading(entry);
        const items = this.findListItemChildren(entry);

        if (items.length > 0) {
            items.forEach(item => this.processBranchNode(item));
        } else {
            this.processLeafNode(entry);
        }

        this.createLinkClickHandlers(entry);
        this.render(entry);
    }
}

Viewport.initOnReady();

export { };
