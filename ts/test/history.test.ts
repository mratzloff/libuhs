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
            const state: HistoryState = { type: "hint", locator: "1" };
            history.pushState(state);

            expect(history.state).toEqual(state);
        });

        it("reflects the current state after navigation", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            history.back();

            expect(history.state).toEqual({ type: "hint", locator: "1" });

            history.forward();

            expect(history.state).toEqual({ type: "hint", locator: "2" });
        });
    });

    describe("pushState", () => {
        it("adds a state", () => {
            const state: HistoryState = { type: "hint", locator: "1" };
            history.pushState(state);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });

        it("adds multiple states", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("does not push duplicate consecutive states", () => {
            const state: HistoryState = { type: "hint", locator: "1" };
            history.pushState(state);
            history.pushState(state);

            expect(history.hasPrevious()).toBe(false);
        });

        it("truncates forward history on push", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.back();
            history.back();
            history.pushState({ type: "hint", locator: "4" });

            expect(history.hasNext()).toBe(false);
            expect(history.hasPrevious()).toBe(true);
        });

        it("does not duplicate when pushing same state after truncate", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.truncate();
            history.pushState({ type: "hint", locator: "1" });

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
            expect(history.state).toEqual({ type: "hint", locator: "1" });
        });
    });

    describe("replaceState", () => {
        it("replaces the current state", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.replaceState({ type: "hint", locator: "2-replaced" });

            history.back();

            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ type: "hint", locator: "1" });

            history.forward();

            const forwardEvent = listener.mock.calls[1][0] as CustomEvent;
            expect(forwardEvent.detail).toEqual({
                type: "hint",
                locator: "2-replaced",
            });
        });

        it("does not change navigation state", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            history.replaceState({ type: "hint", locator: "2-replaced" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("does not truncate forward history", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.back();
            history.replaceState({ type: "hint", locator: "2-replaced" });

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(true);
        });

        it("pushes as initial state when history is empty", () => {
            history.replaceState({ type: "hint", locator: "1" });

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);

            history.pushState({ type: "hint", locator: "2" });
            const listener = vi.fn();
            history.addEventListener("change", listener);
            history.back();

            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ type: "hint", locator: "1" });
        });
    });

    describe("truncate", () => {
        it("removes the current state and forward states", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.back();
            history.truncate();
            history.back();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });

        it("dispatches the previous state on subsequent back", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.truncate();
            history.back();

            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ type: "hint", locator: "1" });
        });

        it("does nothing when history is empty", () => {
            history.truncate();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("back", () => {
        it("moves back one state", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            history.back();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing at the beginning", () => {
            history.pushState({ type: "hint", locator: "1" });

            history.back();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("forward", () => {
        it("moves forward one state", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.back();
            history.back();
            history.forward();

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing at the end", () => {
            history.pushState({ type: "hint", locator: "1" });

            history.forward();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("go", () => {
        it("moves by a positive delta", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.back();
            history.back();
            history.go(2);

            expect(history.hasPrevious()).toBe(true);
            expect(history.hasNext()).toBe(false);
        });

        it("moves by a negative delta", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
            history.pushState({ type: "hint", locator: "3" });

            history.go(-2);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(true);
        });

        it("does nothing when delta is out of bounds", () => {
            history.pushState({ type: "hint", locator: "1" });

            history.go(-5);

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("change event", () => {
        it("dispatches change event on navigation", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            history.back();

            expect(listener).toHaveBeenCalledOnce();
            const event = listener.mock.calls[0][0] as CustomEvent;
            expect(event.detail).toEqual({ type: "hint", locator: "1" });
        });

        it("does not dispatch when navigation is out of bounds", () => {
            const listener = vi.fn();
            history.addEventListener("change", listener);

            history.pushState({ type: "hint", locator: "1" });
            history.back();

            expect(listener).not.toHaveBeenCalled();
        });
    });

    describe("onChange", () => {
        it("fires on pushState", () => {
            const listener = vi.fn();
            history.onChange = listener;

            const state: HistoryState = { type: "hint", locator: "1" };
            history.pushState(state);

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(state, false, false);
        });

        it("fires on replaceState", () => {
            history.pushState({ type: "hint", locator: "1" });

            const listener = vi.fn();
            history.onChange = listener;

            const state: HistoryState = { type: "hint", locator: "1-replaced" };
            history.replaceState(state);

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(state, false, false);
        });

        it("fires on go", () => {
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            const listener = vi.fn();
            history.onChange = listener;
            history.back();

            expect(listener).toHaveBeenCalledOnce();
            expect(listener).toHaveBeenCalledWith(
                { type: "hint", locator: "1" },
                false,
                true,
            );
        });

        it("does not fire when go is out of bounds", () => {
            history.pushState({ type: "hint", locator: "1" });

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

            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });
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
            history.pushState({ type: "hint", locator: "1" });
            history.pushState({ type: "hint", locator: "2" });

            history.clear();

            expect(history.hasPrevious()).toBe(false);
            expect(history.hasNext()).toBe(false);
        });
    });

    describe("session storage resilience", () => {
        it("handles corrupted index gracefully", () => {
            sessionStorage.setItem("history.index", "not-a-number");
            const freshHistory = new History();

            freshHistory.pushState({ type: "hint", locator: "1" });

            expect(freshHistory.hasPrevious()).toBe(false);
        });

        it("handles corrupted states gracefully", () => {
            sessionStorage.setItem("history.states", "not-json");
            const freshHistory = new History();

            freshHistory.pushState({ type: "hint", locator: "1" });

            expect(freshHistory.hasPrevious()).toBe(false);
        });
    });
});
