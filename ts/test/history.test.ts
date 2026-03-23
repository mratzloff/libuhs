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
