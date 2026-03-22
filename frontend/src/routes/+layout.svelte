<script lang="ts">
	import { base } from '$app/paths';
	import { onMount } from 'svelte';
	import favicon from '$lib/assets/favicon.svg';
	import '$lib/app.css';

	let { children } = $props();

	let onGithubPages = $state(false);
	onMount(() => {
		onGithubPages = location.hostname.endsWith('github.io');
	});
</script>

<svelte:head>
	<link rel="icon" href={favicon} />
	<title>laserDESK 2026</title>
</svelte:head>

<div class="ldk-shell">
	<header class="ldk-header">
		<h1>laserDESK 2026</h1>
		<nav class="ldk-nav" aria-label="Main">
			<a href="{base}/usage" data-testid="nav-usage">Usage</a>
			<a href="{base}/workflow" data-testid="nav-workflow">Workflow</a>
			<a href="{base}/rtc" data-testid="nav-rtc">RTC window</a>
		</nav>
	</header>
	{#if onGithubPages}
		<p class="ldk-muted" style="margin:-0.5rem 0 1rem">
			Hosted UI: set <strong>Backend API base URL</strong> below (or on Usage), start <code>laserdesk_backend</code> with
			CORS — see <a href="{base}/usage">Usage</a>. Open the app under <a href="{base}/workflow">Workflow</a>.
		</p>
	{/if}
	{@render children()}
</div>
