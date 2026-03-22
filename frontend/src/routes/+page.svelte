<script lang="ts">
	import { base } from '$app/paths';
	import WorkflowRunner from '$lib/components/WorkflowRunner.svelte';
</script>

<svelte:head>
	<title>Workflow · laserDESK 2026</title>
</svelte:head>

<p class="ldk-muted" style="margin-top:0">
	Reference workflow (mock backend). Steps load
	<code>{base}/workflows/reference-minimal-demo.json</code>
	— schema in repo: <code>docs/workflows/workflow-schema.json</code>.
</p>

<WorkflowRunner />
