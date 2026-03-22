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
	/** Allow Connect unless execution is running (backend may already be connected, e.g. --rtc-demo). */
	let connectDisabled = $derived(busy || connectionState === 'running');

	type StatusKind = 'info' | 'ok' | 'err';
	interface StatusEntry {
		ts: number;
		kind: StatusKind;
		msg: string;
	}
	const STATUS_LOG_MAX = 150;
	let statusLog = $state<StatusEntry[]>([]);

	function pushStatus(kind: StatusKind, msg: string) {
		const line = msg.trim();
		if (!line) return;
		statusLog = [...statusLog.slice(-(STATUS_LOG_MAX - 1)), { ts: Date.now(), kind, msg: line }];
	}

	function fmtStatusTime(ts: number) {
		try {
			return new Date(ts).toLocaleTimeString(undefined, {
				hour: '2-digit',
				minute: '2-digit',
				second: '2-digit',
				hour12: false
			});
		} catch {
			return '';
		}
	}

	let statusLogText = $derived(
		statusLog
			.map(({ ts, kind, msg }) => {
				const tag = kind === 'err' ? 'ERR' : kind === 'ok' ? 'OK ' : 'INFO';
				return `[${fmtStatusTime(ts)}] ${tag}  ${msg}`;
			})
			.join('\n')
	);

	function clearStatusLog() {
		statusLog = [];
	}

	let prevApiErr = $state<string | null>(null);
	let prevSuccess = $state<string | null>(null);
	let prevConn = $state<string | null>(null);
	let prevLoadErr = $state<string | null>(null);

	$effect(() => {
		const e = apiError;
		if (!e) {
			prevApiErr = null;
			return;
		}
		if (e !== prevApiErr) {
			prevApiErr = e;
			pushStatus('err', e);
		}
	});

	$effect(() => {
		const h = successHint;
		if (!h) {
			prevSuccess = null;
			return;
		}
		if (h !== prevSuccess) {
			prevSuccess = h;
			pushStatus('ok', h);
		}
	});

	$effect(() => {
		const le = loadError;
		if (!le) {
			prevLoadErr = null;
			return;
		}
		if (le !== prevLoadErr) {
			prevLoadErr = le;
			pushStatus('err', `Workflow file: ${le}`);
		}
	});

	$effect(() => {
		if (!workflow) {
			prevConn = null;
			return;
		}
		const c = connectionState;
		if (c !== prevConn) {
			prevConn = c;
			pushStatus('info', `Connection state: ${c}`);
		}
	});

	onMount(() => {
		try {
			bc = openRtcChannel();
		} catch {
			bc = null;
		}
		apiBaseDisplay = getApiBase();
		pushStatus('info', `Page origin: ${window.location.origin}`);
		pushStatus('info', `API base: ${getApiBase()}`);
		const onApiBaseChanged = () => {
			apiBaseDisplay = getApiBase();
			pushStatus('info', `API base (updated): ${getApiBase()}`);
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
							disabled={connectDisabled}
							title={busy
								? undefined
								: connectionState === 'running'
									? 'Stop execution before reconnecting'
									: rtcConnected
										? 'Reconnect (resets RTC session) or use Disconnect'
										: undefined}
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

	<div class="ldk-card ldk-status-panel" data-testid="workflow-status-log">
		<div class="ldk-status-panel-head">
			<h2 class="ldk-status-panel-title">Status log</h2>
			<button type="button" class="ldk-btn secondary" data-testid="status-log-clear" onclick={clearStatusLog}
				>Clear log</button
			>
		</div>
		<p class="ldk-muted ldk-status-tip">
			If you already set <code>LASERDESK_CORS_ORIGIN</code> but see “Failed to fetch”: (1) Re-download
			<code>laserdesk_backend.exe</code> from the latest GitHub Release — on start, PowerShell must show
			<code>Access-Control-Allow-Private-Network=true</code>. (2) In Chrome: site lock icon → Permissions / Site
			settings → allow <strong>local network</strong> for this site. (3) Try Edge or Firefox to compare.
		</p>
		<p class="ldk-muted" style="margin:0 0 0.5rem;font-size:0.85rem">
			Current API base: <code data-testid="workflow-api-base">{apiBaseDisplay || '…'}</code>
		</p>
		{#if apiError}
			<p class="ldk-error" role="alert" data-testid="workflow-api-error">{apiError}</p>
		{/if}
		{#if successHint}
			<p class="ldk-success" role="status">{successHint}</p>
		{/if}
		<div role="log" aria-live="polite" aria-relevant="additions" class="ldk-status-log-region">
			<pre class="ldk-pre ldk-status-log-pre" data-testid="workflow-status-log-body">{statusLogText || '(no events yet)'}</pre>
		</div>
	</div>
{/if}

<style>
	.ldk-status-panel {
		margin-top: 1rem;
	}
	.ldk-status-panel-head {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		justify-content: space-between;
		gap: 0.5rem;
		margin-bottom: 0.35rem;
	}
	.ldk-status-panel-title {
		margin: 0;
		font-size: 1.05rem;
	}
	.ldk-status-log-region {
		margin-top: 0.5rem;
	}
	.ldk-status-log-pre {
		margin: 0;
		max-height: 14rem;
		font-size: 0.8rem;
		line-height: 1.35;
		white-space: pre-wrap;
		word-break: break-word;
	}
	.ldk-status-tip {
		margin: 0 0 0.65rem;
		font-size: 0.8rem;
		line-height: 1.4;
		padding: 0.5rem 0.65rem;
		background: #f0f4f8;
		border-radius: 6px;
		border: 1px solid #d8dee6;
	}
</style>
