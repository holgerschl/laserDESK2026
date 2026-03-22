<script lang="ts">
	import { base } from '$app/paths';
	import ApiBaseSettings from '$lib/components/ApiBaseSettings.svelte';
	import WorkflowRunner from '$lib/components/WorkflowRunner.svelte';

	let apiSettingsMinimized = $state(false);
</script>

<svelte:head>
	<title>Workflow · laserDESK 2026</title>
</svelte:head>

<p class="ldk-muted" style="margin-top:0">
	Reference workflow (mock RTC). Steps load
	<code>{base}/workflows/reference-minimal-demo.json</code>
	— schema in repo: <code>docs/workflows/workflow-schema.json</code>.
	<a href="{base}/usage">← Usage & installation</a>
</p>

<ApiBaseSettings bind:minimized={apiSettingsMinimized} autoMinimizeAfterSave />

{#if !apiSettingsMinimized}
	<p class="ldk-muted" style="margin-top:0">
		Hosted UI (e.g. GitHub Pages) talks to the backend on your PC via the URL above. Start
		<code>laserdesk_backend</code> with <code>LASERDESK_CORS_ORIGIN</code> set to your site origin (see Usage), or the
		browser will block requests.
	</p>
{/if}

<WorkflowRunner />
