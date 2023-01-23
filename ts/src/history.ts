class History implements EventTarget {
    readonly indexKey = "history.index";
    readonly statesKey = "history.states";

    private listeners: {[type: string]: ((event: Event) => void)[];} = {};

    constructor() {
        this.clear();
    }

    public addEventListener(type: string, callback: (event: Event) => void): void {
        if (!(type in this.listeners)) {
            this.listeners[type] = [];
        }
        this.listeners[type].push(callback);
    }

    public back(): void {
        this.go(-1);
    }

    public clear(): void {
        sessionStorage.removeItem(this.indexKey);
        sessionStorage.removeItem(this.statesKey);
    }

    public dispatchEvent(event: Event): boolean {
        if (!(event.type in this.listeners)) {
            return true;
        }

        const stack = this.listeners[event.type].slice();
        for (let i = 0; i < stack.length; ++i) {
            stack[i].call(this, event);
        }

        return !event.defaultPrevented;
    }

    public forward(): void {
        this.go(1);
    }

    public go(delta: number): void {
        const index = this.getIndex();
        const states = this.getStates();
        const newIndex = index + delta;

        if (newIndex < 0 || newIndex >= states.length) {
            return;
        }

        this.setIndex(newIndex);
        this.dispatchEvent(new CustomEvent("change", {detail: states[newIndex]}));
    }

    public hasNext(): boolean {
        const index = this.getIndex();
        const states = this.getStates();
        return index + 1 < states.length;
    }

    public hasPrevious(): boolean {
        const index = this.getIndex();
        return index > 0;
    }

    public pushState(state: HistoryState): void {
        const index = this.getIndex();
        let states = this.getStates();
        states = states.slice(0, index + 1);
        states.push(state);
        this.setStates(states);
        this.setIndex(states.length - 1);
    }

    public removeEventListener(type: string, callback: (event: Event) => void): void {
        if (!(type in this.listeners)) {
            return;
        }

        const stack = this.listeners[type];
        for (let i = 0; i < stack.length; ++i) {
            if (stack[i] === callback) {
                stack.splice(i, 1);
                return;
            }
        }
    }

    private getIndex(): number {
        let index = 0;

        const indexValue = sessionStorage.getItem(this.indexKey);
        if (indexValue) {
            const parsed = parseInt(indexValue, 10);
            if (Number.isNaN(parsed)) {
                sessionStorage.removeItem(this.indexKey);
            } else {
                index = parsed;
            }
        }

        return index;
    }

    private getStates(): HistoryState[] {
        let states: HistoryState[] = [];

        const statesValue = sessionStorage.getItem(this.statesKey);
        if (statesValue) {
            try {
                states = JSON.parse(statesValue);
            } catch (error) {
                sessionStorage.removeItem(this.statesKey);
            }
        }

        return states;
    }

    private setIndex(index: number): void {
        sessionStorage.setItem(this.indexKey, index.toString());
    }

    private setStates(states: HistoryState[]): void {
        sessionStorage.setItem(this.statesKey, JSON.stringify(states));
    }
}

export default History;
