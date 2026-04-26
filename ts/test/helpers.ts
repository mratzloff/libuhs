export function clickButton(id: string): void {
    document.getElementById(id)?.click();
}

export function clickFooterButton(): void {
    document.getElementById("button")?.click();
}

export function clickLink(text: string): void {
    const link = Array.from(getViewport().querySelectorAll("a.clickable")).find(
        element => element.textContent === text,
    );
    (link as HTMLElement)?.click();
}

export function clickSearchReset(): void {
    const searchField = getSearchField();
    searchField.value = "";
    searchField.dispatchEvent(new Event("input"));
}

export function clickTitle(text: string): void {
    const title = Array.from(
        getViewport().querySelectorAll(".title.clickable"),
    ).find(element => element.textContent === text);
    (title as HTMLElement)?.click();
}

export function getBreadcrumbTexts(): string[] {
    const breadcrumbs = getViewport().querySelector("ul.breadcrumbs");
    if (!breadcrumbs) {
        return [];
    }

    return Array.from(breadcrumbs.querySelectorAll("li")).map(
        li => li.textContent ?? "",
    );
}

export function getClickableTitles(): string[] {
    return Array.from(getViewport().querySelectorAll(".title.clickable")).map(
        element => element.textContent ?? "",
    );
}

export function getFooterButtonText(): string | null {
    return document.getElementById("button")?.textContent ?? null;
}

export function getHeading(): string | null {
    return getViewport().querySelector("h1")?.textContent ?? null;
}

export function getHiddenHintCount(): number {
    const items = getViewport().querySelectorAll("ol > li");

    return Array.from(items).filter(item => (item as HTMLElement).hidden)
        .length;
}

export function getLinks(): string[] {
    return Array.from(getViewport().querySelectorAll("a.clickable")).map(
        element => element.textContent ?? "",
    );
}

export function getSearchField(): HTMLInputElement {
    return document.querySelector("input[type='search']") as HTMLInputElement;
}

export function getTitles(): string[] {
    return Array.from(getViewport().querySelectorAll(".title")).map(
        element => element.textContent ?? "",
    );
}

export function getViewport(): HTMLElement {
    return document.getElementById("viewport")!;
}

export function getVisibleHints(): string[] {
    const items = getViewport().querySelectorAll("ol > li");

    return Array.from(items)
        .filter(item => !(item as HTMLElement).hidden)
        .map(item => item.textContent?.trim() ?? "");
}

export function hasBlankElement(): boolean {
    return getViewport().querySelector("hr") !== null;
}

export function typeSearch(value: string): void {
    const searchField = getSearchField();
    searchField.value = value;
    searchField.dispatchEvent(
        new InputEvent("input", { inputType: "insertText" }),
    );
}
