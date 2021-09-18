class Viewport {
    private viewport: HTMLElement;

    public static initOnReady() {
        document.addEventListener("DOMContentLoaded", () => new Viewport);
    }

    public constructor() {
        // Create viewport
        this.viewport = document.createElement("div");
        this.viewport.id = "viewport";
        document.body.appendChild(this.viewport);

        // Go to initial entry point
        const rootDocument = document.querySelector(`#root > section[data-type="document"]`);
        const version = rootDocument?.getAttribute("data-version");
        const id = (version === "88a") ? "5" : "1";
        this.view(id);

        // Support back button
        window.addEventListener("popstate", (event) => this.view(event.state ?? id));
    }

    private cloneEntryPoint(id: string): HTMLElement {
        const originalEntry = document.getElementById(id);
        if (!originalEntry) {
            throw new Error("could not find entry point");
        }

        return originalEntry.cloneNode(true) as HTMLElement;
    }

    private createFooter(items: HTMLElement[]): void {
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
        const buttonText = (items.length == 1) ? "Back" : "Show next hint";
        button.textContent = buttonText;
        button.addEventListener("click", () => this.onButtonClick(items));
        footer.appendChild(button);

        document.body.appendChild(footer);
    }

    private createLinkClickHandlers(container: HTMLElement): void {
        const links = container.querySelectorAll("a[href]:not(.hyperlink), area[href]");
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
                link.addEventListener("click", () => this.go(targetId), {capture: false});
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

            overlay.addEventListener("click", () => this.showOverlay(targetId), {capture: false});
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
            title.addEventListener("click", () => this.go(id), {capture: false});
            title.classList.add("clickable");
        });
    }

    private findListItemChildren(container: HTMLElement): NodeListOf<HTMLElement> {
        return container.querySelectorAll(":scope > ol > li") as NodeListOf<HTMLElement>;
    }

    private go(id: string): void {
        window.history.pushState(id, "");
        this.view(id);
    }

    private onButtonClick(items: HTMLElement[]): void {
        for (let i = 0; i < items.length; ++i) {
            const item = items[i];
            if (!item.hidden) {
                continue;
            }

            item.removeAttribute("hidden");
            this.updateHintProgress((i + 1) / items.length);
            window.scrollTo(0, document.body.scrollHeight);

            if (i + 1 == items.length) {
                this.setButtonText("Back");
            }

            return;
        }

        this.removeFooter();
        window.history.back();
    }

    private processHintNode(container: HTMLElement): void {
        const type = container.getAttribute("data-type");
        if (type != "hint" && type != "nesthint") {
            return;
        }

        const items = Array.from(this.findListItemChildren(container));
        for (const item of items.slice(1)) {
            item.hidden = true;
        }

        this.createFooter(items);
        this.updateHintProgress(1 / items.length);
    }

    private processLeafNode(container: HTMLElement): void {
        this.replaceIdsWithDataAttributes(container);
        this.updateImageMapAnchor(container);
        this.createOverlayClickHandler(container);
    }

    private processListNode(container: HTMLElement): void {
        if (!container.firstChild) {
            throw new Error("empty list item");
        }

        this.removeUnnecessaryElements(container);
        this.replaceIdsWithDataAttributes(container);
        this.createTitleClickHandlers(container);
    }

    private removeFooter(): void {
        const footer = document.getElementById("footer");
        footer?.remove();
    }

    private removeUnnecessaryElements(container: HTMLElement): void {
        container.querySelector("p:not(.title)")?.remove();
        const mediaFiles = container.querySelectorAll(".media");
        mediaFiles.forEach(mediaFile => mediaFile.remove());
        const lists = container.querySelectorAll("ol");
        lists.forEach(list => list.remove());
    }

    private render(container: HTMLElement): void {
        this.viewport.appendChild(container);
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

    private reset(): void {
        while (this.viewport.lastChild) {
            this.viewport.removeChild(this.viewport.lastChild);
        }
        this.removeFooter();
    }

    private setButtonText(text: string): void {
        const button = document.getElementById("button");
        if (!button) {
            throw new Error("could not find button");
        }
        button.textContent = text;
    }

    private showOverlay(id: string): void {
        const overlay = this.viewport.querySelector(`[data-id="${id}"]`);
        if (!overlay) {
            throw new Error("could not find overlay");
        }
        overlay.removeAttribute("hidden");
    }

    private updateHintProgress(value: number): void {
        const progress = document.getElementById("progress");
        progress?.style.setProperty("width", `${value * 100}%`);
    }

    private updateImageMapAnchor(container: HTMLElement): void {
        const map = container.querySelector("map");
        if (!map) {
            return;
        }

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

    private view(id: string): void {
        this.reset();

        const entry = this.cloneEntryPoint(id);
        this.replaceTitleWithHeading(entry);
        const items = this.findListItemChildren(entry);

        if (items.length > 0) {
            items.forEach(item => this.processListNode(item));
        } else {
            this.processLeafNode(entry);
        }

        this.createLinkClickHandlers(entry);
        this.processHintNode(entry);
        this.render(entry);
    }
}

Viewport.initOnReady();

export { };
