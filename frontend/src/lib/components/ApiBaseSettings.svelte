<script lang="ts">
	import { browser } from '$app/environment';
	import { onMount } from 'svelte';
	import {
		API_BASE_STORAGE_KEY,
		DEFAULT_LOCAL_BACKEND_API_BASE,
		getApiBase,
		LASERDESK_API_BASE_CHANGED_EVENT
	} from '$lib/api/config';

	interface Props {
		/** When true, collapse after a successful save and if a value is already stored (workflow page). */
		autoMinimizeAfterSave?: boolean;
		/** Bind so the parent can hide surrounding help text while minimized. */
		minimized?: boolean;
	}
	let {
		autoMinimizeAfterSave = false,
		minimized = $bindable(false)
	}: Props = $props();

	let input = $state(DEFAULT_LOCAL_BACKEND_API_BASE);
	let saved = $state(false);
	let saveError = $state<string | null>(null);
	let savedClearTimer: ReturnType<typeof setTimeout> | null = null;
	let stripNotice = $state<string | null>(null);
	let stripNoticeTimer: ReturnType<typeof setTimeout> | null = null;
	let displayUrl = $state('');

	function syncDisplayUrl() {
		displayUrl = getApiBase();
	}

	function flashStripNotice(msg: string, ms = 4000) {
		stripNotice = msg;
		if (stripNoticeTimer) clearTimeout(stripNoticeTimer);
		stripNoticeTimer = setTimeout(() => {
			stripNotice = null;
			stripNoticeTimer = null;
		}, ms);
	}

	onMount(() => {
		try {
			const v = localStorage.getItem(API_BASE_STORAGE_KEY);
			if (v?.trim()) input = v.trim();
			else input = getApiBase();
		} catch {
			input = getApiBase();
		}
		syncDisplayUrl();
		if (autoMinimizeAfterSave) {
			try {
				if (localStorage.getItem(API_BASE_STORAGE_KEY)?.trim()) {
					minimized = true;
					syncDisplayUrl();
				}
			} catch {
				/* ignore */
			}
		}

		const onRemote = () => syncDisplayUrl();
		window.addEventListener(LASERDESK_API_BASE_CHANGED_EVENT, onRemote);
		return () => window.removeEventListener(LASERDESK_API_BASE_CHANGED_EVENT, onRemote);
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
			notifyApiBaseChanged();
			syncDisplayUrl();
			if (autoMinimizeAfterSave) {
				if (savedClearTimer) {
					clearTimeout(savedClearTimer);
					savedClearTimer = null;
				}
				saved = false;
				minimized = true;
				flashStripNotice('Saved.');
			} else {
				flashSaved();
			}
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
			notifyApiBaseChanged();
			syncDisplayUrl();
			if (autoMinimizeAfterSave) {
				if (savedClearTimer) {
					clearTimeout(savedClearTimer);
					savedClearTimer = null;
				}
				saved = false;
				minimized = true;
				flashStripNotice('Reset to default.');
			} else {
				flashSaved();
			}
		} catch {
			saveError =
				'Could not clear saved URL (private window or storage blocked).';
		}
	}

	function expand() {
		minimized = false;
		stripNotice = null;
	}
</script>

{#if minimized}
	<div class="ldk-api-strip" data-testid="api-base-minimized">
		<div class="ldk-api-strip-main">
			<span class="ldk-muted">Backend API</span>
			<code class="ldk-api-strip-url">{displayUrl}</code>
			<button type="button" class="ldk-btn secondary" data-testid="api-base-change" onclick={expand}
				>Change</button
			>
		</div>
		{#if stripNotice}
			<p class="ldk-success strip-notice" role="status" aria-live="polite">{stripNotice}</p>
		{/if}
	</div>
{:else}
	<div class="ldk-card" style="margin-bottom:0.65rem">
		<h2 style="margin:0 0 0.35rem;font-size:0.98rem">Backend API base URL</h2>
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
{/if}

<style>
	.api-base-saved-banner {
		margin: 0.45rem 0 0;
		padding: 0.4rem 0.55rem;
		border-radius: 6px;
		border: 1px solid #8abf9b;
		background: #e8f5ea;
		font-weight: 500;
	}

	.ldk-api-strip {
		margin-bottom: 1rem;
		padding: 0.5rem 0.75rem;
		border-radius: 6px;
		border: 1px solid #d8dee6;
		background: #fafbfc;
	}

	.ldk-api-strip-main {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 0.4rem 0.55rem;
	}

	.ldk-api-strip-url {
		font-size: 0.9rem;
		word-break: break-all;
	}

	.strip-notice {
		margin: 0.35rem 0 0;
		font-weight: 500;
	}
</style>
