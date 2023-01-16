const DEFAULT_BOOST = 1;
const TITLE_BOOST = 10;

interface Match {
    element: HTMLElement;
    score: number;
}

function search(keywords: string): HTMLElement[] {
    const results: HTMLElement[] = [];
    const matchMap = new Map<String, Match>;
    const needle = keywords.trim().toLowerCase();

    const root = document.getElementById("root");
    if (!root) {
        return results;
    }

    const elements = root.querySelectorAll("[data-type]");
    elements.forEach(element => {
        if (element.hasAttribute("hidden")) {
            return;
        }

        const title = element.querySelector(":scope > div > .title");
        const haystack = title?.textContent?.toLowerCase();
        if (haystack?.includes(needle)) {
            const id = element.getAttribute("data-id");
            if (id) {
                const match = matchMap.get(id);
                if (match) {
                    match.score += TITLE_BOOST;
                } else {
                    matchMap.set(id, {
                        element: element as HTMLElement,
                        score: TITLE_BOOST,
                    });
                }
            }
        }

        if (element.getAttribute("data-type") == "text") {
            const body = element.querySelector(":scope > p");
            const haystack = body?.textContent?.toLowerCase();
            const pattern = new RegExp(needle);
            const matches = haystack?.match(pattern) || [];
            const numMatches = matches.length;

            if (numMatches > 0) {
                const id = element.getAttribute("data-id");
                if (id) {
                    const match = matchMap.get(id);
                    if (match) {
                        match.score += (numMatches * DEFAULT_BOOST);
                    } else {
                        matchMap.set(id, {
                            element: element as HTMLElement,
                            score: numMatches * DEFAULT_BOOST,
                        });
                    }
                }
            }
        } else {
            const hints = element.querySelectorAll(":scope > ol > li > .hint");
            hints?.forEach(hint => {
                const haystack = hint.textContent?.toLowerCase();
                if (haystack?.includes(needle)) {
                    const id = element.getAttribute("data-id");
                    if (id) {
                        const match = matchMap.get(id);
                        if (match) {
                            match.score += DEFAULT_BOOST;
                        } else {
                            matchMap.set(id, {
                                element: element as HTMLElement,
                                score: DEFAULT_BOOST,
                            });
                        }
                    }
                }
            });
        }
    });

    const unsorted: Match[] = [];
    matchMap.forEach(match => unsorted.push(match));
    const sorted = unsorted.sort((a, b) => b.score - a.score);
    sorted.forEach(match => results.push(match.element));

    return results;
}

export default search;
