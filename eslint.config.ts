import eslintConfigPrettier from "eslint-config-prettier";
import perfectionist from "eslint-plugin-perfectionist";
import simpleImportSort from "eslint-plugin-simple-import-sort";
import tseslint from "typescript-eslint";

import { defineConfig } from "@eslint/config-helpers";
import eslint from "@eslint/js";

export default defineConfig([
    eslint.configs.recommended,
    tseslint.configs.recommended,
    eslintConfigPrettier,
    {
        files: ["ts/src/**/*.ts", "ts/test/**/*.ts"],
        languageOptions: {
            parserOptions: {
                projectService: true,
                tsconfigRootDir: new URL(".", import.meta.url).pathname,
            },
        },
        plugins: {
            perfectionist,
            "simple-import-sort": simpleImportSort,
        },
        rules: {
            "perfectionist/sort-enums": "error",
            "perfectionist/sort-interfaces": "error",
            "perfectionist/sort-object-types": "error",
            "perfectionist/sort-objects": "error",
            "simple-import-sort/exports": "error",
            "simple-import-sort/imports": "error",
        },
    },
    {
        ignores: ["_deps/**", "build/**", "CMakeFiles/**", "node_modules/**"],
    },
]);
