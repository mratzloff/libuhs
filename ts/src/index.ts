import { publishAPI } from "./uhs";
import Viewport from "./viewport";

document.addEventListener("DOMContentLoaded", () => {
    const viewport = new Viewport();
    publishAPI(viewport);
});
