import { defineConfig } from "vitest/config";

export default defineConfig({
    test: {
        environment: "jsdom",
        include: ["ts/test/**/*.test.ts"],
    },
});
