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
			<a href="{base}/">Workflow</a>
			<a href="{base}/rtc" data-testid="nav-rtc">RTC window</a>
			<a href="{base}/usage">Usage</a>
		</nav>
	</header>
	{#if onGithubPages}
		<p class="ldk-muted" style="margin:-0.5rem 0 1rem">
			Hosted UI: configure your PC backend URL on the
			<a href="{base}/usage">Usage</a> page and start <code>laserdesk_backend</code> with CORS (see there).
		</p>
	{/if}
	{@render children()}
</div>
