/**
 * Comprehensive fixture matching real HTML writer output.
 *
 *   document (96a)
 *   ├── [hidden] document (88a, visibility=none)
 *   │   └── subject#header "–"
 *   │       └── hint#header-hint "–?"
 *   │           └── "–"
 *   ├── subject#1 "Shadowed Passage Atlas"              ← HOME
 *   │   ├── subject#3 "The Cellar"
 *   │   │   ├── credit#5 "Who wrote this section?"
 *   │   │   │   body: "Compiled by River Ashford."
 *   │   │   ├── nesthint#8 "How do I open the trapdoor?"
 *   │   │   │   ├── "Look for scratches on the floor."
 *   │   │   │   ├── "Pull the iron ring."
 *   │   │   │   └── "It leads to the crypt."
 *   │   │   ├── blank#13
 *   │   │   ├── hint#15 "Is there a key?"
 *   │   │   │   └── "Under the loose flagstone."
 *   │   │   └── link#18 → #21 "See also: The Tower"
 *   │   ├── subject#21 "The Tower"
 *   │   │   ├── nesthint#23 "How do I reach the top?"
 *   │   │   │   ├── "Take the spiral staircase."
 *   │   │   │   └── "Watch for the collapsed step."
 *   │   │   ├── text#28 "Tower layout"
 *   │   │   │   body: "The tower has three floors."
 *   │   │   └── link#31 → #3 "See also: The Cellar"
 *   │   └── nesthint#34 "FAQ"
 *   │       ├── "Read the manual."
 *   │       └── "Ask the innkeeper."
 *   ├── [hidden] version#39 (visibility=none)
 *   └── [hidden] info#41 (visibility=none)
 */
export function buildFixture(): void {
    document.body.innerHTML = `
<main id="root" hidden="">
<section data-type="document" data-version="96a">
<ol>
<li hidden="">
<section data-type="document" data-version="88a" data-visibility="none">
<ol>
<li>
<div data-id="99" data-type="subject">
<div><span class="title">-</span></div>
<ol>
<li>
<div data-id="98" data-type="hint">
<div><span class="title">-?</span></div>
<ol>
<li><div class="hint"><p><span>-</span></p></div></li>
</ol>
</div>
</li>
</ol>
</div>
</li>
</ol>
</section>
</li>
<li>
<div data-id="1" id="1" data-type="subject">
<div><span class="title">Shadowed Passage Atlas</span></div>
<ol>
<li>
<div data-id="3" id="3" data-type="subject">
<div><span class="title">The Cellar</span></div>
<ol>
<li>
<div data-id="5" id="5" data-type="credit">
<div><span class="title">Who wrote this section?</span></div>
<p>Compiled by River Ashford.</p>
</div>
</li>
<li>
<div data-id="8" id="8" data-type="nesthint">
<div><span class="title">How do I open the trapdoor?</span></div>
<ol>
<li><div class="hint"><p><span>Look for scratches on the floor.</span></p></div></li>
<li><div class="hint"><p><span>Pull the iron ring.</span></p></div></li>
<li><div class="hint"><p><span>It leads to the crypt.</span></p></div></li>
</ol>
</div>
</li>
<li>
<hr data-id="13" id="13" data-type="blank" />
</li>
<li>
<div data-id="15" id="15" data-type="hint">
<div><span class="title">Is there a key?</span></div>
<ol>
<li><div class="hint"><p><span>Under the loose flagstone.</span></p></div></li>
</ol>
</div>
</li>
<li>
<a data-id="18" id="18" data-type="link" data-target="21" href="#21">See also: The Tower</a>
</li>
</ol>
</div>
</li>
<li>
<div data-id="21" id="21" data-type="subject">
<div><span class="title">The Tower</span></div>
<ol>
<li>
<div data-id="23" id="23" data-type="nesthint">
<div><span class="title">How do I reach the top?</span></div>
<ol>
<li><div class="hint"><p><span>Take the spiral staircase.</span></p></div></li>
<li><div class="hint"><p><span>Watch for the collapsed step.</span></p></div></li>
</ol>
</div>
</li>
<li>
<div data-id="28" id="28" data-type="text">
<div><span class="title">Tower layout</span></div>
<p>The tower has three floors.</p>
</div>
</li>
<li>
<a data-id="31" id="31" data-type="link" data-target="3" href="#3">See also: The Cellar</a>
</li>
</ol>
</div>
</li>
<li>
<div data-id="34" id="34" data-type="nesthint">
<div><span class="title">FAQ</span></div>
<ol>
<li><div class="hint"><p><span>Read the manual.</span></p></div></li>
<li><div class="hint"><p><span>Ask the innkeeper.</span></p></div></li>
</ol>
</div>
</li>
</ol>
</div>
</li>
<li hidden="">
<div data-id="39" id="39" data-type="version" data-visibility="none">
<div><span class="title">96a</span></div>
</div>
</li>
<li hidden="">
<div data-id="41" id="41" data-type="info" data-visibility="none" id="info">
<dl><dt>Date Created</dt><dd>2026-03-20</dd></dl>
</div>
</li>
</ol>
</section>
</main>`;
}
