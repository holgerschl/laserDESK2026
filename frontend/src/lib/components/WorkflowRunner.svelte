<script lang="ts">
	import { base } from '$app/paths';
	import { onDestroy, onMount } from 'svelte';
	import * as api from '$lib/api/laserdesk';
	import { getApiBase, LASERDESK_API_BASE_CHANGED_EVENT } from '$lib/api/config';
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
	let successHint = $state<string | null>(null);
	let busy = $state(false);
	let jobId = $state<string | null>(null);
	let paramValues = $state<Record<string, string>>({});
	let rtcJson = $state<string>('');
	let healthJson = $state<string>('');

	let bc: ReturnType<typeof openRtcChannel> | null = null;

	let apiBaseDisplay = $state('');

	let connectionState = $derived.by(() => {
		if (!rtcJson) return '—';
		try {
			const o = JSON.parse(rtcJson) as { connection_state?: string };
			return o.connection_state ?? '—';
		} catch {
			return '—';
		}
	});

	let rtcConnected = $derived(connectionState !== 'disconnected' && connectionState !== '—');
	let jobRunning = $derived(connectionState === 'running');
	let canStartJob = $derived(connectionState === 'loaded');

	onMount(() => {
		try {
			bc = openRtcChannel();
		} catch {
			bc = null;
		}
		apiBaseDisplay = getApiBase();
		const onApiBaseChanged = () => {
			apiBaseDisplay = getApiBase();
		};
		window.addEventListener(LASERDESK_API_BASE_CHANGED_EVENT, onApiBaseChanged);
		void loadWorkflow();
		return () => window.removeEventListener(LASERDESK_API_BASE_CHANGED_EVENT, onApiBaseChanged);
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
			try {
				await fetchStatusOnly();
			} catch (e) {
				apiError = e instanceof Error ? e.message : String(e);
			}
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
		successHint = null;
		busy = true;
		try {
			return await fn();
		} catch (e) {
			apiError = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	async function fetchStatusOnly(): Promise<void> {
		const [h, s] = await Promise.all([api.getHealth(), api.getRtcStatus()]);
		healthJson = JSON.stringify(h, null, 2);
		rtcJson = JSON.stringify(s, null, 2);
	}

	async function doConnectMock() {
		await withBusy(async () => {
			await api.postRtcConnect({ mode: 'mock' });
			log('RTC connect: mock');
			await fetchStatusOnly();
			successHint = 'Connected (mock).';
		});
	}

	async function doDisconnect() {
		await withBusy(async () => {
			await api.postRtcDisconnect();
			jobId = null;
			log('RTC disconnect');
			await fetchStatusOnly();
			successHint = 'Disconnected.';
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
			await fetchStatusOnly();
			successHint = `Job registered (${r.job_id}). You can start execution.`;
		});
	}

	async function doRun() {
		await withBusy(async () => {
			await api.postMinimalDemoRun();
			log('Run: start');
			await fetchStatusOnly();
			successHint = 'Execution started.';
		});
	}

	async function doStop() {
		await withBusy(async () => {
			await api.postMinimalDemoStop();
			log('Run: stop');
			await fetchStatusOnly();
			successHint = 'Execution stopped.';
		});
	}

	async function refreshStatus() {
		await withBusy(async () => {
			await fetchStatusOnly();
			successHint = 'Status refreshed.';
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
	<p class="ldk-muted" data-testid="workflow-api-base">
		API base: <code>{apiBaseDisplay || '…'}</code>
	</p>

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
	{#if successHint}
		<p class="ldk-success" role="status" aria-live="polite">{successHint}</p>
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
					<p class="ldk-muted" style="margin-top:0">
						Connection: <strong data-testid="connect-step-state">{connectionState}</strong>
					</p>
					<div class="ldk-row">
						<button
							type="button"
							class="ldk-btn"
							data-testid="connect-mock"
							disabled={busy || rtcConnected}
							title={busy ? undefined : rtcConnected ? 'Already connected' : undefined}
							onclick={() => doConnectMock()}>Connect (mock)</button
						>
						<button
							type="button"
							class="ldk-btn secondary"
							data-testid="disconnect-rtc"
							disabled={busy || !rtcConnected}
							title={busy ? undefined : !rtcConnected ? 'Not connected' : undefined}
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
					<p class="ldk-muted" style="margin-top:0">
						Execution: <strong data-testid="run-step-state">{jobRunning ? 'running' : 'stopped'}</strong>
						<span class="ldk-muted"> · RTC: {connectionState}</span>
					</p>
					<div class="ldk-row">
						<button
							type="button"
							class="ldk-btn"
							data-testid="start-run"
							disabled={busy || jobRunning || !canStartJob}
							title={busy
								? undefined
								: jobRunning
									? 'Already running'
									: !rtcConnected
										? 'Connect first'
										: !canStartJob
											? 'Load a job first'
											: undefined}
							onclick={() => doRun()}>Start</button
						>
						<button
							type="button"
							class="ldk-btn danger"
							data-testid="stop-run"
							disabled={busy || !jobRunning}
							title={busy ? undefined : !jobRunning ? 'Not running' : undefined}
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
