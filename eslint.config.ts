import eslintConfigPrettier from "eslint-config-prettier";
import tseslint from "typescript-eslint";

import {defineConfig} from "@eslint/config-helpers";
import eslint from "@eslint/js";

export default defineConfig([
    eslint.configs.recommended,
    tseslint.configs.recommended,
    eslintConfigPrettier,
    {
        files: ["ts/src/**/*.ts"],
        languageOptions: {
            parserOptions: {
                projectService: true,
                tsconfigRootDir: new URL(".", import.meta.url).pathname,
            },
        },
    },
    {
        ignores: ["build/**", "node_modules/**"],
    },
]);
