import { beforeEach, describe, expect, it, vi } from "vitest";

import { History, HistoryState } from "../src/history";

describe("History", () => {
    let history: History;

    beforeEach(() => {
        sessionStorage.clear();
        history = new History();
    });

    describe("constructor", () => {
        it("clears session storage on construction", () => {
            sessionStorage.setItem("history.index", "5");
            sessionStorage.setItem("history.states", "[]");

            new History();

            expect(sessionStorage.getItem("history.index")).toBeNull();
            expect(sessionStorage.getItem("history.states")).toBeNull();
        });
    });

    describe("state", () => {
        it("returns null when history is empty", () => {
            expect(history.state).toBeNull();
        });

        it("returns the current state after pushState", () => {
            const state: HistoryState = { locator: "1", type: "hint" };
            history.pushState(state);

            expect(history.state).toEqual(state);
        });

        it("reflects the current state after navigation", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            history.back();

            expect(history.state).toEqual({ locator: "1", type: "hint" });

            history.forward();

            expect(history.state).toEqual({ locator: "2", type: "hint" });
        });
    });

    describe("pushState", () => {
        it("adds a state", () => {
            const state: HistoryState = { locator: "1", type: "hint" };
            history.pushState(state);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });

        it("adds multiple states", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("does not push duplicate consecutive states", () => {
            const state: HistoryState = { locator: "1", type: "hint" };
            history.pushState(state);
            history.pushState(state);

            expect(history.hasPrevious()).toBe(false);
        });

        it("truncates forward history on push", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            history.back();
            history.back();
            history.pushState({ locator: "4", type: "hint" });

            expect(history.hasNext()).toBe(false);
            expect(history.hasPrevious()).toBe(true);
        });
    });

    describe("replaceState", () => {
        it("replaces the current state", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.replaceState({ locator: "2-replaced", type: "hint" });

            history.back();

            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ locator: "1", type: "hint" });

            history.forward();

            const forwardEvent = listener.mock.calls[1][0] as CustomEvent;
            expect(forwardEvent.detail).toEqual({
                locator: "2-replaced",
                type: "hint",
            });
        });

        it("does not change navigation state", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            history.replaceState({ locator: "2-replaced", type: "hint" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("does not truncate forward history", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            history.back();
            history.replaceState({ locator: "2-replaced", type: "hint" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(true);
        });

        it("pushes as initial state when history is empty", () => {
            history.replaceState({ locator: "1", type: "hint" });

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);

            history.pushState({ locator: "2", type: "hint" });
            const listener = vi.fn();
            history.addEventListener("change", listener);
            history.back();

            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ locator: "1", type: "hint" });
        });
    });

    describe("back", () => {
        it("moves back one state", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            history.back();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing at the beginning", () => {
            history.pushState({ locator: "1", type: "hint" });

            history.back();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("forward", () => {
        it("moves forward one state", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            history.back();
            history.back();
            history.forward();

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing at the end", () => {
            history.pushState({ locator: "1", type: "hint" });

            history.forward();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("go", () => {
        it("moves by a positive delta", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            history.back();
            history.back();
            history.go(2);

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("moves by a negative delta", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.pushState({ locator: "3", type: "hint" });

            history.go(-2);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing when delta is out of bounds", () => {
            history.pushState({ locator: "1", type: "hint" });

            history.go(-5);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("change event", () => {
        it("dispatches change event on navigation", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            history.back();

            expect(listener).toHaveBeenCalledOnce();
            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ locator: "1", type: "hint" });
        });

        it("does not dispatch when navigation is out of bounds", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ locator: "1", type: "hint" });
            history.back();

            expect(listener).not.toHaveBeenCalled();
        });
    });

    describe("onChange", () => {
        it("fires on pushState", () => {
            const listener = vi.fn();
            history.onChange = listener;

            const state: HistoryState = { locator: "1", type: "hint" };
            history.pushState(state);

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(state, false, false);
        });

        it("fires on replaceState", () => {
            history.pushState({ locator: "1", type: "hint" });

            const listener = vi.fn();
            history.onChange = listener;

            const state: HistoryState = { locator: "1-replaced", type: "hint" };
            history.replaceState(state);

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(state, false, false);
        });

        it("fires on go", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            const listener = vi.fn();
            history.onChange = listener;
            history.back();

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(
                { locator: "1", type: "hint" },
                false,
                true,
            );
        });

        it("does not fire when go is out of bounds", () => {
            history.pushState({ locator: "1", type: "hint" });

            const listener = vi.fn();
            history.onChange = listener;
            history.back();

            expect(listener).not.toHaveBeenCalled();
        });
    });

    describe("removeEventListener", () => {
        it("removes a listener", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);
            history.removeEventListener("change", listener);

            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });
            history.back();

            expect(listener).not.toHaveBeenCalled();
        });

        it("does nothing for unregistered type", () => {
            const listener = vi.fn();
            history.removeEventListener("nonexistent", listener);
        });
    });

    describe("clear", () => {
        it("resets all state", () => {
            history.pushState({ locator: "1", type: "hint" });
            history.pushState({ locator: "2", type: "hint" });

            history.clear();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("session storage resilience", () => {
        it("handles corrupted index gracefully", () => {
            sessionStorage.setItem("history.index", "not-a-number");
            const freshHistory = new History();

            freshHistory.pushState({ locator: "1", type: "hint" });

            expect(freshHistory.hasPrevious()).toBe(false);
        });

        it("handles corrupted states gracefully", () => {
            sessionStorage.setItem("history.states", "not-json");
            const freshHistory = new History();

            freshHistory.pushState({ locator: "1", type: "hint" });

            expect(freshHistory.hasPrevious()).toBe(false);
        });
    });
});
