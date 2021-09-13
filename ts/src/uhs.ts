function init() {
    const root = document.getElementById("root");
    if (!root) {
        throw new Error("could not find document root");
    }
    root.setAttribute("hidden", "true");

    const viewport = document.getElementById("viewport");
    if (!viewport) {
        throw new Error("could not find viewport");
    }
    viewport.removeAttribute("hidden");

    view("1", viewport);
}

function view(id: string, viewport: HTMLElement) {
    // Reset viewport
    while (viewport.lastChild) {
        viewport.removeChild(viewport.lastChild);
    }

    const entry = document.getElementById(id);
    if (!entry) {
        throw new Error("could not find entry point");
    }

    const title = entry.querySelector(":scope > p.title");
    if (!title || !title.textContent) {
        throw new Error("could not find section title");
    }
    const h1 = document.createElement("h1");
    h1.appendChild(document.createTextNode(title.textContent));
    viewport.appendChild(h1);

    const items = entry.querySelectorAll(":scope > ol > li");
    items.forEach(item => {
        const copy = item.cloneNode(true) as HTMLLIElement;
        if (!copy.firstElementChild) {
            throw new Error("empty list item");
        }
        const id = copy.firstElementChild.id;
        copy.querySelector("p:not(.title)")?.remove();
        copy.querySelector("ol")?.remove();
        copy.addEventListener("click", () => view(id, viewport));
        viewport.appendChild(copy);
    });
}

document.addEventListener("DOMContentLoaded", init);

export { };
