<script lang="ts">
	import { base } from '$app/paths';
	import ApiBaseSettings from '$lib/components/ApiBaseSettings.svelte';
	import WorkflowRunner from '$lib/components/WorkflowRunner.svelte';
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

<details class="ldk-workflow-details" open data-testid="workflow-api-details">
	<summary class="ldk-workflow-details-summary">API connection (backend URL)</summary>
	<div class="ldk-workflow-details-body">
		<ApiBaseSettings />
		<p class="ldk-muted" style="margin:0">
			Hosted UI (e.g. GitHub Pages) talks to the backend on your PC. Start
			<code>laserdesk_backend</code> with <code>LASERDESK_CORS_ORIGIN</code> set to this page’s origin (scheme + host
			only — see Usage), and use a backend build that supports local-network CORS. Status messages appear in the log
			below the workflow.
		</p>
	</div>
</details>

<WorkflowRunner />

<style>
	.ldk-workflow-details {
		margin-bottom: 1rem;
		padding: 0.5rem 0.75rem;
		border: 1px solid #d8dee6;
		border-radius: 6px;
		background: #fafbfc;
	}
	.ldk-workflow-details-summary {
		cursor: pointer;
		font-weight: 600;
		padding: 0.25rem 0;
	}
	.ldk-workflow-details-body {
		margin-top: 0.5rem;
		padding-top: 0.5rem;
		border-top: 1px solid #e8ecf0;
	}
</style>
