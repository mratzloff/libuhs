class History implements EventTarget {
    readonly indexKey = "history.index";
    readonly stateKey = "history.state";

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
        window.sessionStorage.removeItem(this.indexKey);
        window.sessionStorage.removeItem(this.stateKey);
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
        const state = this.getState();
        const newIndex = index + delta;

        if (newIndex < 0 || newIndex == state.length) {
            return;
        }

        this.setIndex(newIndex);
        this.dispatchEvent(new CustomEvent("change", {detail: state[newIndex]}));
    }

    public hasNext(): boolean {
        const index = this.getIndex();
        const state = this.getState();
        return index + 1 < state.length;
    }

    public hasPrevious(): boolean {
        const index = this.getIndex();
        return index > 0;
    }

    public pushState(id: string): void {
        const index = this.getIndex();
        let state = this.getState();
        state = state.slice(0, index + 1);
        state.push(id);
        this.setState(state);
        this.setIndex(state.length - 1);
    }

    private getIndex(): number {
        let index = 0;

        const indexValue = window.sessionStorage.getItem(this.indexKey);
        if (indexValue) {
            const parsed = parseInt(indexValue, 10);
            if (parsed === NaN) {
                window.sessionStorage.removeItem(this.indexKey);
            } else {
                index = parsed;
            }
        }

        return index;
    }

    private getState(): string[] {
        let state: string[] = [];

        const stateValue = window.sessionStorage.getItem(this.stateKey);
        if (stateValue) {
            try {
                state = JSON.parse(stateValue);
            } catch (error) {
                window.sessionStorage.removeItem(this.stateKey);
            }
        }

        return state;
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

    private setIndex(index: number): void {
        window.sessionStorage.setItem(this.indexKey, index.toString());
    }

    private setState(state: string[]): void {
        window.sessionStorage.setItem(this.stateKey, JSON.stringify(state));
    }
}

export default History;
