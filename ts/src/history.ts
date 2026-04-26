export type HistoryChangeCallback =
    | ((
          state: HistoryState | null,
          hasPrevious: boolean,
          hasNext: boolean,
      ) => void)
    | null;

export interface HistoryState {
    type: string;
    locator: string;
}

class History implements EventTarget {
    readonly indexKey = "history.index";
    readonly statesKey = "history.states";

    constructor() {
        this.clear();
    }

    public onChange: HistoryChangeCallback = null;

    public get state(): HistoryState | null {
        const index = this.getIndex();
        const states = this.getStates();
        return states[index] ?? null;
    }

    public addEventListener(
        type: string,
        callback: (event: Event) => void,
    ): void {
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
        this.dispatchEvent(
            new CustomEvent("change", { detail: states[newIndex] }),
        );
        this.notifyChange();
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

        // Don't push same state twice
        const last = states[states.length - 1];
        if (last && this.isStateEqual(last, state)) {
            states = states.slice(0, -1);
        }

        states.push(state);
        this.setStates(states);
        this.setIndex(states.length - 1);
        this.notifyChange();
    }

    public removeEventListener(
        type: string,
        callback: (event: Event) => void,
    ): void {
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

    public replaceState(state: HistoryState): void {
        const states = this.getStates();
        if (states.length == 0) {
            states.push(state);
        } else {
            const index = this.getIndex();
            states[index] = state;
        }
        this.setStates(states);
        this.notifyChange();
    }

    public truncate(): void {
        const index = this.getIndex();
        const states = this.getStates();
        if (index < 0 || index >= states.length) {
            return;
        }
        states.splice(index);
        this.setStates(states);
    }

    private listeners: { [type: string]: ((event: Event) => void)[] } = {};

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
            } catch {
                sessionStorage.removeItem(this.statesKey);
            }
        }

        return states;
    }

    private isStateEqual(a: HistoryState, b: HistoryState): boolean {
        return a.type == b.type && a.locator == b.locator;
    }

    private notifyChange(): void {
        this.onChange?.(this.state, this.hasPrevious(), this.hasNext());
    }

    private setIndex(index: number): void {
        sessionStorage.setItem(this.indexKey, index.toString());
    }

    private setStates(states: HistoryState[]): void {
        sessionStorage.setItem(this.statesKey, JSON.stringify(states));
    }
}

export { History };
