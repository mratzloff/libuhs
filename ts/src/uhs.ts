import { HistoryChangeCallback } from "./history";
import Viewport from "./viewport";

interface API {
    back: () => void;
    forward: () => void;
    hasNext: () => boolean;
    hasPrevious: () => boolean;
    home: () => void;
    onHistoryChange: HistoryChangeCallback;
    search: (keywords: string) => void;
    title: string;
}

declare global {
    interface Window {
        uhs: API;
    }
}

export function publishAPI(viewport: Viewport): void {
    const title = findTitle();
    document.title = title;

    window.uhs = {
        back: () => viewport.back(),
        forward: () => viewport.forward(),
        hasNext: () => viewport.hasNext(),
        hasPrevious: () => viewport.hasPrevious(),
        home: () => viewport.home(),
        get onHistoryChange() {
            return viewport.onHistoryChange;
        },
        set onHistoryChange(callback: HistoryChangeCallback) {
            viewport.onHistoryChange = callback;
        },
        search: (keywords: string) => viewport.search(keywords),
        title,
    };
}

function findTitle(): string {
    const element = document.querySelector(
        '#root div[data-id="1"] > div > span.title',
    );
    return element?.textContent ?? "";
}
