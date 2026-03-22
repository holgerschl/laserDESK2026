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
	<h1 style="margin-top:0">Usage and installation</h1>
	<p class="ldk-muted">
		laserDESK 2026 is split into a <strong>browser UI</strong> (Svelte Kit) and a <strong>local REST backend</strong>
		(<code>laserdesk_backend</code>) that talks to mock or real RTC. Source repository:
		<a href="https://github.com/holgerschl/laserDESK2026">github.com/holgerschl/laserDESK2026</a>.
	</p>

	<h2>Hosted web app (GitHub Pages)</h2>
	<p>
		The public UI is published at
		<a href="https://holgerschl.github.io/laserDESK2026/">https://holgerschl.github.io/laserDESK2026/</a>
		(adjust if your repository name differs). This page is the default landing; the workflow UI is under
		<a href="{base}/workflow">Workflow</a>. The backend does <strong>not</strong> run on GitHub — it must run on your
		computer (or lab PC) while you use the site.
	</p>
	<ol>
		<li>
			<strong>Download the Windows backend:</strong>
			<a href={RELEASE_EXE} data-testid="download-backend-exe">laserdesk_backend.exe (latest Release)</a>
			— direct link. All releases:
			<a href={RELEASES_LATEST}>GitHub Releases</a>.
			<span class="ldk-muted"
				>GitHub shows a <strong>404 page</strong> until at least one Release exists. Fix:
				<a href={RELEASE_WORKFLOW}>open the “Release Windows backend” workflow</a> and click
				<strong>Run workflow</strong> (publishes <code>laserdesk_backend.exe</code>), or push a version tag such as
				<code>v0.1.0</code>.</span
			>
		</li>
		<li>
			Open PowerShell in the folder containing the executable and run (example — use your GitHub Pages origin):
			<pre class="ldk-pre">$env:LASERDESK_CORS_ORIGIN = "https://holgerschl.github.io"
.\laserdesk_backend.exe --port 8080</pre>
		</li>
		<li>
			On this page, set <strong>Backend API base URL</strong> (below) to
			<code>http://127.0.0.1:8080/api/v1</code> and save.
		</li>
		<li>
			Open <a href="{base}/workflow">Workflow</a> and run the reference flow (Connect mock → load job → run).
		</li>
	</ol>
	<p class="ldk-muted">
		CORS is required because the page is served from <code>github.io</code> while the API is on <code>localhost</code>.
		Only use <code>LASERDESK_CORS_ORIGIN</code> with the exact Pages URL you trust; omit it for local-only use.
	</p>

	<h2>Backend executable (Windows)</h2>
	<p>
		CI attaches <code>laserdesk_backend.exe</code> to <a href={RELEASES_LATEST}>Releases</a>. Download again:
		<a href={RELEASE_EXE}>latest <code>laserdesk_backend.exe</code></a>. To build from source, see
		<code>backend/README.md</code> in the repository (CMake, C++17).
	</p>

	<h2>Local development</h2>
	<ol>
		<li>Build and run the backend: <code>laserdesk_backend --port 8080</code> (optional <code>--rtc-demo</code>).</li>
		<li>
			From <code>frontend/</code>: <code>npm install</code>, then <code>npm run dev</code> (Vite proxies
			<code>/api</code> to the backend; set <code>LASERDESK_BACKEND_URL</code> if not using port 8080).
		</li>
		<li>Open the printed dev URL; API defaults to same-origin <code>/api/v1</code>.</li>
	</ol>

	<h2>Configure API URL (browser)</h2>
	<p class="ldk-muted">
		When the UI and API are not on the same origin, the saved URL below is used for all REST calls. It is stored only
		in this browser (localStorage).
	</p>
	<ApiBaseSettings />

	<h2>RTC monitor and BroadcastChannel</h2>
	<p>
		The <a href="{base}/rtc">RTC window</a> shows health/status and log lines sent from the workflow view via
		<code>BroadcastChannel</code> <code>laserdesk-rtc-v1</code>. Open both routes in the same browser profile.
	</p>

	<h2>Further reading</h2>
	<ul>
		<li><a href="https://github.com/holgerschl/laserDESK2026/blob/main/README.md">Repository README</a></li>
		<li><a href="https://github.com/holgerschl/laserDESK2026/blob/main/docs/implementation-plan.md">Implementation plan</a></li>
		<li><a href="https://github.com/holgerschl">github.com/holgerschl</a> — account overview</li>
	</ul>
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
</style>
