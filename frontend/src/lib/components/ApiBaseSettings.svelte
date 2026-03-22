<script lang="ts">
	import { onMount } from 'svelte';
	import { API_BASE_STORAGE_KEY, DEFAULT_LOCAL_BACKEND_API_BASE, getApiBase } from '$lib/api/config';

	let input = $state(DEFAULT_LOCAL_BACKEND_API_BASE);
	let saved = $state(false);

	onMount(() => {
		try {
			const v = localStorage.getItem(API_BASE_STORAGE_KEY);
			if (v?.trim()) input = v.trim();
			else input = getApiBase();
		} catch {
			input = getApiBase();
		}
	});

	function save() {
		const v = input.trim().replace(/\/$/, '');
		try {
			localStorage.setItem(API_BASE_STORAGE_KEY, v);
			saved = true;
			setTimeout(() => (saved = false), 2500);
		} catch {
			/* ignore */
		}
	}

	function clearStored() {
		try {
			localStorage.removeItem(API_BASE_STORAGE_KEY);
			input = DEFAULT_LOCAL_BACKEND_API_BASE;
			saved = true;
			setTimeout(() => (saved = false), 2500);
		} catch {
			/* ignore */
		}
	}
</script>

<div class="ldk-card" style="margin-bottom:1rem">
	<h2 style="margin:0 0 0.5rem;font-size:1.05rem">Backend API base URL</h2>
	<p class="ldk-muted" style="margin-top:0">
		Must end with <code>/api/v1</code> (no trailing slash after <code>v1</code>). Required when the UI is
		served from GitHub Pages and the backend runs on your PC.
	</p>
	<div class="ldk-field" style="max-width:36rem">
		<label for="api-base">API root</label>
		<input
			id="api-base"
			type="url"
			bind:value={input}
			placeholder={DEFAULT_LOCAL_BACKEND_API_BASE}
		/>
	</div>
	<div class="ldk-row">
		<button type="button" class="ldk-btn" onclick={save}>Save in this browser</button>
		<button type="button" class="ldk-btn secondary" onclick={clearStored}>Reset to default</button>
	</div>
	{#if saved}
		<p class="ldk-muted" style="margin-bottom:0">Saved. Reload the workflow page if it was already open.</p>
	{/if}
</div>
