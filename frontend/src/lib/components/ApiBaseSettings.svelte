<script lang="ts">
	import { browser } from '$app/environment';
	import { onMount } from 'svelte';
	import {
		API_BASE_STORAGE_KEY,
		DEFAULT_LOCAL_BACKEND_API_BASE,
		getApiBase,
		LASERDESK_API_BASE_CHANGED_EVENT
	} from '$lib/api/config';

	let input = $state(DEFAULT_LOCAL_BACKEND_API_BASE);
	let saved = $state(false);
	let saveError = $state<string | null>(null);
	let savedClearTimer: ReturnType<typeof setTimeout> | null = null;

	onMount(() => {
		try {
			const v = localStorage.getItem(API_BASE_STORAGE_KEY);
			if (v?.trim()) input = v.trim();
			else input = getApiBase();
		} catch {
			input = getApiBase();
		}
	});

	function notifyApiBaseChanged() {
		if (browser) {
			window.dispatchEvent(new CustomEvent(LASERDESK_API_BASE_CHANGED_EVENT));
		}
	}

	function flashSaved() {
		saveError = null;
		saved = true;
		if (savedClearTimer) clearTimeout(savedClearTimer);
		savedClearTimer = setTimeout(() => {
			saved = false;
			savedClearTimer = null;
		}, 6000);
	}

	function save() {
		const v = input.trim().replace(/\/$/, '');
		saveError = null;
		if (!v) {
			saveError = 'Enter a URL.';
			return;
		}
		if (!v.endsWith('/api/v1')) {
			saveError = 'URL must end with /api/v1 (no trailing slash after v1).';
			return;
		}
		try {
			localStorage.setItem(API_BASE_STORAGE_KEY, v);
			flashSaved();
			notifyApiBaseChanged();
		} catch {
			saveError =
				'Could not save (private window or storage blocked). Allow storage for this site or use a normal window.';
		}
	}

	function clearStored() {
		saveError = null;
		try {
			localStorage.removeItem(API_BASE_STORAGE_KEY);
			input = DEFAULT_LOCAL_BACKEND_API_BASE;
			flashSaved();
			notifyApiBaseChanged();
		} catch {
			saveError =
				'Could not clear saved URL (private window or storage blocked).';
		}
	}
</script>

<div class="ldk-card" style="margin-bottom:1rem">
	<h2 style="margin:0 0 0.5rem;font-size:1.05rem">Backend API base URL</h2>
	<p class="ldk-muted" style="margin-top:0">
		Must end with <code>/api/v1</code> (no trailing slash after <code>v1</code>). On GitHub Pages the default
		already points at your PC; change this only if you use another port or host.
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
		<button type="button" class="ldk-btn" data-testid="api-base-save" onclick={save}>Save in this browser</button>
		<button type="button" class="ldk-btn secondary" data-testid="api-base-reset" onclick={clearStored}
			>Reset to default</button
		>
	</div>
	{#if saveError}
		<p class="ldk-error" role="alert" data-testid="api-base-error">{saveError}</p>
	{/if}
	{#if saved}
		<p
			class="ldk-success api-base-saved-banner"
			role="status"
			aria-live="polite"
			data-testid="api-base-saved"
		>
			Saved. The app will use this address for API calls right away.
		</p>
	{/if}
</div>

<style>
	.api-base-saved-banner {
		margin: 0.75rem 0 0;
		padding: 0.5rem 0.75rem;
		border-radius: 6px;
		border: 1px solid #8abf9b;
		background: #e8f5ea;
		font-weight: 500;
	}
</style>
