<script lang="ts">
	import { base } from '$app/paths';
	import ApiBaseSettings from '$lib/components/ApiBaseSettings.svelte';

	const RELEASE_EXE =
		'https://github.com/holgerschl/laserDESK2026/releases/latest/download/laserdesk_backend.exe';
	const RELEASES_LATEST = 'https://github.com/holgerschl/laserDESK2026/releases/latest';
	const RELEASE_WORKFLOW =
		'https://github.com/holgerschl/laserDESK2026/actions/workflows/release-backend-windows.yml';
</script>

<svelte:head>
	<title>Usage & installation · laserDESK 2026</title>
</svelte:head>

<article class="ldk-doc">
	<h1 style="margin-top:0">Usage</h1>
	<p class="ldk-muted" style="margin-top:0">
		You use the app in the browser. A small program (<code>laserdesk_backend</code>) must run on <strong>your
			PC</strong> (Windows) so the page can talk to it — you do not need to host the website yourself.
	</p>

	<h2>Quick start</h2>
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

	<h2>Only if you need it</h2>
	<p class="ldk-muted" style="margin-top:0">
		Change the line below if your backend is not on port 8080 or not at <code>127.0.0.1</code>. Saved in this browser
		only.
	</p>
	<ApiBaseSettings />

	<details class="ldk-doc-details">
		<summary>Download 404, developers, RTC window</summary>
		<p class="ldk-muted">
			<strong>404 on the exe link:</strong> no release is published yet. Check
			<a href={RELEASES_LATEST}>Releases</a> or trigger
			<a href={RELEASE_WORKFLOW}>Release Windows backend</a> in Actions. Building from source:
			<code>backend/README.md</code> in the repo.
		</p>
		<p class="ldk-muted">
			<strong>Local development:</strong> run the backend on 8080, then from <code>frontend/</code> run
			<code>npm install</code> and <code>npm run dev</code> — the dev server proxies <code>/api</code> (see repo
			README).
		</p>
		<p class="ldk-muted">
			<strong>RTC window</strong> (<a href="{base}/rtc">open</a>): extra health/status and logs from the workflow via
			the browser’s <code>BroadcastChannel</code>; use the same browser profile for both tabs.
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
