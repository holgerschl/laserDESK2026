<script lang="ts">
	import { base } from '$app/paths';
	import { onDestroy, onMount } from 'svelte';
	import * as api from '$lib/api/laserdesk';
	import { openRtcChannel, postRtcLog } from '$lib/laser/rtcChannel';
	import type { WorkflowDefinition, WorkflowStep } from '$lib/workflow/types';
	import { assertValidWorkflow } from '$lib/workflow/validate';

	interface Props {
		workflowUrl?: string;
	}
	let { workflowUrl = `${base}/workflows/reference-minimal-demo.json` }: Props = $props();

	let workflow = $state<WorkflowDefinition | null>(null);
	let loadError = $state<string | null>(null);
	let stepIndex = $state(0);
	let apiError = $state<string | null>(null);
	let busy = $state(false);
	let jobId = $state<string | null>(null);
	let paramValues = $state<Record<string, string>>({});
	let rtcJson = $state<string>('');
	let healthJson = $state<string>('');

	let bc: ReturnType<typeof openRtcChannel> | null = null;

	let connectionState = $derived.by(() => {
		if (!rtcJson) return '—';
		try {
			const o = JSON.parse(rtcJson) as { connection_state?: string };
			return o.connection_state ?? '—';
		} catch {
			return '—';
		}
	});

	onMount(() => {
		try {
			bc = openRtcChannel();
		} catch {
			bc = null;
		}
		void loadWorkflow();
	});

	onDestroy(() => {
		try {
			bc?.close();
		} catch {
			/* ignore */
		}
	});

	async function loadWorkflow() {
		loadError = null;
		try {
			const res = await fetch(workflowUrl);
			if (!res.ok) throw new Error(`HTTP ${res.status}`);
			const data: unknown = await res.json();
			const w = assertValidWorkflow(data);
			workflow = w;
			const next: Record<string, string> = {};
			for (const step of w.steps) {
				for (const f of step.parameterFields ?? []) {
					if (f.default !== undefined && f.default !== null) next[f.id] = String(f.default);
					else next[f.id] = '';
				}
			}
			paramValues = next;
			postRtcLog(bc, `Workflow loaded: ${w.id} v${w.version}`);
		} catch (e) {
			loadError = e instanceof Error ? e.message : String(e);
		}
	}

	function currentStep(): WorkflowStep | null {
		const w = workflow;
		if (!w || stepIndex < 0 || stepIndex >= w.steps.length) return null;
		return w.steps[stepIndex]!;
	}

	function log(line: string) {
		postRtcLog(bc, line);
	}

	async function withBusy<T>(fn: () => Promise<T>): Promise<T | void> {
		apiError = null;
		busy = true;
		try {
			return await fn();
		} catch (e) {
			apiError = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	async function doConnectMock() {
		await withBusy(async () => {
			await api.postRtcConnect({ mode: 'mock' });
			log('RTC connect: mock');
			await refreshStatus();
		});
	}

	async function doDisconnect() {
		await withBusy(async () => {
			await api.postRtcDisconnect();
			jobId = null;
			log('RTC disconnect');
			await refreshStatus();
		});
	}

	function labelForJob(): string | undefined {
		const v = paramValues['label']?.trim();
		return v || undefined;
	}

	async function doLoadJob() {
		await withBusy(async () => {
			const r = await api.postMinimalDemoJob(labelForJob());
			jobId = r.job_id;
			log(`Job registered: ${r.job_id}`);
			await refreshStatus();
		});
	}

	async function doRun() {
		await withBusy(async () => {
			await api.postMinimalDemoRun();
			log('Run: start');
			await refreshStatus();
		});
	}

	async function doStop() {
		await withBusy(async () => {
			await api.postMinimalDemoStop();
			log('Run: stop');
			await refreshStatus();
		});
	}

	async function refreshStatus() {
		await withBusy(async () => {
			const [h, s] = await Promise.all([api.getHealth(), api.getRtcStatus()]);
			healthJson = JSON.stringify(h, null, 2);
			rtcJson = JSON.stringify(s, null, 2);
		});
	}
</script>

{#if loadError}
	<p class="ldk-error" role="alert">Failed to load workflow: {loadError}</p>
	<button type="button" class="ldk-btn secondary" onclick={() => loadWorkflow()}>Retry</button>
{:else if !workflow}
	<p class="ldk-muted">Loading workflow…</p>
{:else}
	<p class="ldk-muted">{workflow.description}</p>

	<div class="ldk-steps" role="tablist" aria-label="Workflow steps">
		{#each workflow.steps as s, i (s.id)}
			<button
				type="button"
				class="ldk-step-tab"
				aria-current={i === stepIndex ? 'step' : undefined}
				onclick={() => (stepIndex = i)}
			>
				{s.title}
			</button>
		{/each}
	</div>

	{#if apiError}
		<p class="ldk-error" role="alert">{apiError}</p>
	{/if}

	<div class="ldk-card">
		{#key stepIndex}
			{@const step = currentStep()}
			{#if step}
				<h2 style="margin:0 0 0.5rem;font-size:1.1rem">{step.title}</h2>
				{#if step.description}
					<p class="ldk-muted" style="margin-top:0">{step.description}</p>
				{/if}

				{#if step.kind === 'connect'}
					<div class="ldk-row">
						<button
							type="button"
							class="ldk-btn"
							data-testid="connect-mock"
							disabled={busy}
							onclick={() => doConnectMock()}>Connect (mock)</button
						>
						<button
							type="button"
							class="ldk-btn secondary"
							data-testid="disconnect-rtc"
							disabled={busy}
							onclick={() => doDisconnect()}>Disconnect</button
						>
					</div>
				{:else if step.kind === 'parameters'}
					{#each step.parameterFields ?? [] as f (f.id)}
						<div class="ldk-field">
							<label for="wf-{f.id}">{f.label}</label>
							<input
								id="wf-{f.id}"
								data-testid="param-{f.id}"
								type="text"
								value={paramValues[f.id] ?? ''}
								oninput={(e) => {
									const v = e.currentTarget.value;
									paramValues = { ...paramValues, [f.id]: v };
								}}
							/>
						</div>
					{/each}
				{:else if step.kind === 'load-job'}
					<button
						type="button"
						class="ldk-btn"
						data-testid="load-job"
						disabled={busy}
						onclick={() => doLoadJob()}>Register minimal demo job</button
					>
					{#if jobId}
						<p class="ldk-muted" data-testid="job-id">Job id: {jobId}</p>
					{/if}
				{:else if step.kind === 'run'}
					<div class="ldk-row">
						<button
							type="button"
							class="ldk-btn"
							data-testid="start-run"
							disabled={busy}
							onclick={() => doRun()}>Start</button
						>
						<button
							type="button"
							class="ldk-btn danger"
							data-testid="stop-run"
							disabled={busy}
							onclick={() => doStop()}>Stop</button
						>
					</div>
				{:else if step.kind === 'status'}
					<div class="ldk-row" style="margin-bottom:0.75rem">
						<button
							type="button"
							class="ldk-btn secondary"
							data-testid="refresh-status"
							disabled={busy}
							onclick={() => refreshStatus()}>Refresh status</button
						>
					</div>
					<p class="ldk-muted" data-testid="connection-state">
						Connection: <strong>{connectionState}</strong>
					</p>
					{#if healthJson}
						<h3 style="font-size:0.85rem;margin:0.75rem 0 0.25rem">Health</h3>
						<pre class="ldk-pre" data-testid="health-json">{healthJson}</pre>
					{/if}
					{#if rtcJson}
						<h3 style="font-size:0.85rem;margin:0.75rem 0 0.25rem">RTC status</h3>
						<pre class="ldk-pre" data-testid="rtc-json">{rtcJson}</pre>
					{/if}
				{:else}
					<p class="ldk-muted">No UI for step kind “{step.kind}”.</p>
				{/if}
			{/if}
		{/key}
	</div>
{/if}
