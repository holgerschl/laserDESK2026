<script lang="ts">
	import { base } from '$app/paths';
	import ApiBaseSettings from '$lib/components/ApiBaseSettings.svelte';

	const RELEASE_EXE =
		'https://github.com/holgerschl/laserDESK2026/releases/latest/download/laserdesk_backend.exe';
	const RELEASE_MACOS_TAR =
		'https://github.com/holgerschl/laserDESK2026/releases/latest/download/laserdesk_backend-macos.tar.gz';
	const RELEASES_LATEST = 'https://github.com/holgerschl/laserDESK2026/releases/latest';
	const RELEASE_WORKFLOW =
		'https://github.com/holgerschl/laserDESK2026/actions/workflows/release-backend.yml';
</script>

<svelte:head>
	<title>Usage & installation · laserDESK 2026</title>
</svelte:head>

<article class="ldk-doc">
	<h1 style="margin-top:0">Usage</h1>
	<p class="ldk-muted" style="margin-top:0">
		You use the app in the browser. A small program (<code>laserdesk_backend</code>) must run on <strong>your
			computer</strong> so the page can talk to it — you do not need to host the website yourself. On Windows, use
		the pre-built <code>.exe</code> from Releases; on a MacBook, use
		<a href={RELEASE_MACOS_TAR}>laserdesk_backend-macos.tar.gz</a> or build from source (section below).
	</p>

	<h2>Quick start (Windows)</h2>
	<ol>
		<li>
			<strong>Download</strong>
			<a href={RELEASE_EXE} data-testid="download-backend-exe">laserdesk_backend.exe</a>
			(Windows, from GitHub Releases).
		</li>
		<li>
			<strong>Start the backend</strong> — PowerShell, folder with the exe. Set
			<code>LASERDESK_CORS_ORIGIN</code> to the <strong>same origin</strong> as the site in your address bar (scheme +
			host, no path). For the public demo that is usually:
			<pre class="ldk-pre">$env:LASERDESK_CORS_ORIGIN = "https://holgerschl.github.io"
.\laserdesk_backend.exe --port 8080</pre>
			Leave the window open while you use the app.
		</li>
		<li>
			<strong>Use the workflow</strong> — open <a href="{base}/workflow">Workflow</a>, then
			<em>Connect (mock)</em> → register job → <em>Start</em>. (Default API address is already
			<code>http://127.0.0.1:8080/api/v1</code> for the hosted app.)
		</li>
	</ol>
	<p class="ldk-muted">
		The website and the backend are on different addresses; CORS tells the browser that combination is allowed. Only
		use an origin you trust.
	</p>

	<h2>MacBook (macOS)</h2>
	<p class="ldk-muted" style="margin-top:0">
		The UI is the same (<a href="{base}/workflow">Workflow</a> on the hosted site or local dev). Full build details:
		<a href="https://github.com/holgerschl/laserDESK2026/blob/main/backend/README.md">backend/README.md</a>.
	</p>
	<ol>
		<li>
			<strong>Download</strong>
			<a href={RELEASE_MACOS_TAR} data-testid="download-backend-macos-tar">laserdesk_backend-macos.tar.gz</a>
			(<strong>Apple Silicon</strong> arm64 build from GitHub Actions; Intel Mac: build from source below). Extract it next to where you want to run
			from, e.g.:
			<pre class="ldk-pre">tar xzf laserdesk_backend-macos.tar.gz</pre>
			This creates <code>laserdesk_backend-macos</code> (executable bit preserved).
		</li>
		<li>
			<strong>Run the backend</strong> — if you use the <strong>hosted</strong> app in the browser, set CORS to your
			page origin (same as Windows). Example for the public demo:
			<pre class="ldk-pre">cd /path/to/folder/with/the/binary
export LASERDESK_CORS_ORIGIN="https://holgerschl.github.io"
./laserdesk_backend-macos --port 8080</pre>
			Leave the terminal open while you use the site. For <strong>local UI only</strong>, you can omit
			<code>LASERDESK_CORS_ORIGIN</code> and use the optional dev step below.
		</li>
		<li>
			<strong>Build from source instead</strong> — clone the repo, install Xcode Command Line Tools and CMake, then:
			<pre class="ldk-pre">xcode-select --install
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build --parallel</pre>
			Binary: <code>backend/build/laserdesk_backend</code>. Or use
			<code>chmod +x scripts/run-backend-macos.sh</code> and
			<pre class="ldk-pre">export LASERDESK_CORS_ORIGIN="https://holgerschl.github.io"
./scripts/run-backend-macos.sh</pre>
		</li>
		<li>
			<strong>Optional — run the web UI on the Mac:</strong> in another terminal,
			<pre class="ldk-pre">cd frontend
npm install
npm run dev</pre>
			Open the printed URL; the dev server proxies <code>/api</code> to <code>http://127.0.0.1:8080</code>. Override
			with <code>LASERDESK_BACKEND_URL</code> if the backend uses another port.
		</li>
		<li>
			<strong>Use the workflow</strong> — same as Windows: open <a href="{base}/workflow">Workflow</a> (hosted) with
			API base <code>http://127.0.0.1:8080/api/v1</code>, or use the local dev URL after <code>npm run dev</code>.
		</li>
	</ol>
	<p class="ldk-muted">
		Safari and Chrome may ask for permission to reach the local network when the page is HTTPS (e.g. GitHub Pages) and
		the API is on <code>localhost</code>. Allow it, or use local <code>npm run dev</code> to avoid cross-origin calls.
	</p>

	<h2>Only if you need it</h2>
	<p class="ldk-muted" style="margin-top:0">
		Change the line below if your backend is not on port 8080 or not at <code>127.0.0.1</code>. Saved in this browser
		only.
	</p>
	<ApiBaseSettings />

	<h2>How to see RTC activity (telegrams) in the log</h2>
	<p class="ldk-muted" style="margin-top:0">
		The <a href="{base}/rtc">RTC window</a> shows an <strong>activity log</strong> at the bottom. That log is
		<strong>not</strong> a live dump of UDP packets from the RTC6 board. It only shows short text lines that the
		<strong>Workflow</strong> or <strong>DXF demo</strong> page sends through the browser’s
		<code>BroadcastChannel</code> (<code>laserdesk-rtc-v1</code>) when you connect, load a job, start, or stop.
	</p>
	<ol>
		<li>
			Open <a href="{base}/rtc">RTC</a> in one tab and keep it open.
		</li>
		<li>
			In a <strong>second tab</strong> on the <strong>same site</strong> (same address and path prefix, e.g.
			<code>{base}/workflow</code> or <code>{base}/dxf</code>), run <em>Connect</em>, load, <em>Start</em>, etc.
		</li>
		<li>
			Lines should appear in the RTC tab’s log with timestamps. Many entries mention the <strong>logical</strong>
			Remote Interface command the backend would use in <strong>ethernet</strong> mode (for example
			<code>R_DC_EXECUTE_LIST_POS</code> on start). That is a human-readable hint, not the raw telegram bytes.
		</li>
	</ol>
	<p class="ldk-muted">
		<strong>Mock RTC</strong> (<em>Connect (mock)</em>) does not send Ethernet telegrams at all; the backend only
		simulates state. <strong>Ethernet</strong> mode sends real telegrams to the board; the UI still does not display
		hex payloads unless we add a separate debug feature later.
	</p>

	<details class="ldk-doc-details">
		<summary>Download 404, developers, RTC window</summary>
		<p class="ldk-muted">
			<strong>404 on a download link:</strong> no release is published yet. Check
			<a href={RELEASES_LATEST}>Releases</a> or trigger
			<a href={RELEASE_WORKFLOW}>Release backend</a> in Actions (Windows <code>.exe</code> + macOS
			<code>.tar.gz</code>). Building from source: <code>backend/README.md</code> in the repo.
		</p>
		<p class="ldk-muted">
			<strong>Local development:</strong> run the backend on 8080, then from <code>frontend/</code> run
			<code>npm install</code> and <code>npm run dev</code> — the dev server proxies <code>/api</code> (see repo
			README).
		</p>
		<p class="ldk-muted">
			<strong>RTC window</strong> (<a href="{base}/rtc">open</a>): health/status polling and the cross-tab activity
			log. See the section <strong>How to see RTC activity (telegrams) in the log</strong> above.
		</p>
		<ul>
			<li><a href="https://github.com/holgerschl/laserDESK2026">Repository</a></li>
			<li><a href="https://github.com/holgerschl/laserDESK2026/blob/main/README.md">README</a></li>
			<li><a href="https://github.com/holgerschl/laserDESK2026/blob/main/docs/implementation-plan.md">Implementation plan</a></li>
		</ul>
	</details>
</article>

<style>
	.ldk-doc h2 {
		font-size: 1.15rem;
		margin-top: 1.5rem;
		border-bottom: 1px solid #d8dee6;
		padding-bottom: 0.25rem;
	}
	.ldk-doc ol,
	.ldk-doc ul {
		padding-left: 1.25rem;
	}
	.ldk-doc li {
		margin-bottom: 0.35rem;
	}
	.ldk-doc-details {
		margin-top: 1.5rem;
		padding: 0.75rem 1rem;
		border: 1px solid #d8dee6;
		border-radius: 6px;
		background: #fafbfc;
	}
	.ldk-doc-details summary {
		cursor: pointer;
		font-weight: 600;
	}
	.ldk-doc-details > :not(summary) {
		margin-top: 0.75rem;
	}
</style>
