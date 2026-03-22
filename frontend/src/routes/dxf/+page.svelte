<script lang="ts">
	import { base } from '$app/paths';
	import * as api from '$lib/api/laserdesk';
	import type { DxfJobEntity, DxfJobResponse } from '$lib/api/laserdesk';

	let busy = $state(false);
	let err = $state<string | null>(null);
	let hint = $state<string | null>(null);
	let jobId = $state<string | null>(null);
	let job = $state<DxfJobResponse | null>(null);
	let selectedIndex = $state<number | null>(null);
	let rtcState = $state<string>('—');
	let dxfLineCount = $state<number | null>(null);

	function clearHints() {
		err = null;
		hint = null;
	}

	async function withBusy(fn: () => Promise<void>) {
		clearHints();
		busy = true;
		try {
			await fn();
		} catch (e) {
			err = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	async function refreshRtc() {
		try {
			const s = await api.getRtcStatus();
			rtcState = s.connection_state ?? '—';
			dxfLineCount = s.dxf_line_count ?? null;
		} catch {
			rtcState = '—';
			dxfLineCount = null;
		}
	}

	function svgLayout(entities: DxfJobEntity[]) {
		let minX = Infinity;
		let minY = Infinity;
		let maxX = -Infinity;
		let maxY = -Infinity;
		for (const e of entities) {
			minX = Math.min(minX, e.x0, e.x1);
			maxX = Math.max(maxX, e.x0, e.x1);
			minY = Math.min(minY, e.y0, e.y1);
			maxY = Math.max(maxY, e.y0, e.y1);
		}
		const padX = Math.max((maxX - minX) * 0.08, 1);
		const padY = Math.max((maxY - minY) * 0.08, 1);
		const w = maxX - minX + 2 * padX;
		const h = maxY - minY + 2 * padY;
		const flip = (yw: number) => maxY + minY - yw;
		return { minX: minX - padX, minY: minY - padY, maxX: maxX + padX, maxY: maxY + padY, w, h, flip };
	}

	$effect(() => {
		void refreshRtc();
	});

	async function connectMock() {
		await withBusy(async () => {
			await api.postRtcConnect({ mode: 'mock' });
			await refreshRtc();
			hint = 'Connected (mock).';
		});
	}

	async function disconnectRtc() {
		await withBusy(async () => {
			await api.postRtcDisconnect();
			jobId = null;
			job = null;
			await refreshRtc();
			hint = 'Disconnected.';
		});
	}

	async function loadDemoFromStatic() {
		await withBusy(async () => {
			const url = `${base}/demo/dxf/SCANLABLogo.dxf`;
			const res = await fetch(url);
			if (!res.ok) throw new Error(`Failed to fetch demo DXF (${res.status})`);
			const text = await res.text();
			const r = await api.postJobsDxfText(text, 'SCANLABLogo.dxf');
			jobId = r.job_id;
			job = await api.getJobsDxf(r.job_id);
			selectedIndex = null;
			hint = `Parsed ${job.line_count} LINE entities.`;
		});
	}

	async function loadDemoFromServer() {
		await withBusy(async () => {
			const r = await api.postJobsDxfDemo();
			jobId = r.job_id;
			job = await api.getJobsDxf(r.job_id);
			selectedIndex = null;
			hint = `Parsed ${job.line_count} LINE entities (server file).`;
		});
	}

	async function loadIntoRtc() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfLoad(id);
			await refreshRtc();
			hint = 'DXF job loaded into RTC session.';
		});
	}

	async function startRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfRun(id);
			await refreshRtc();
			hint = 'Execution started.';
		});
	}

	async function stopRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfStop(id);
			await refreshRtc();
			hint = 'Execution stopped.';
		});
	}

	let layout = $derived(job?.entities?.length ? svgLayout(job.entities) : null);
	let running = $derived(rtcState === 'running');
	let loaded = $derived(rtcState === 'loaded');
</script>

<svelte:head>
	<title>DXF demo · laserDESK 2026</title>
</svelte:head>

<article class="ldk-doc">
	<h1 style="margin-top:0">Phase G – DXF demo</h1>
	<p class="ldk-muted" style="margin-top:0">
		Load the reference <code>SCANLABLogo.dxf</code> (LINE entities), preview geometry, register the parsed job with the
		mock RTC, then start/stop execution. Backend API:
		<code>/api/v1/jobs/dxf</code>.
	</p>

	{#if err}
		<p class="ldk-error" role="alert" data-testid="dxf-page-error">{err}</p>
	{/if}
	{#if hint}
		<p class="ldk-success" role="status" data-testid="dxf-page-hint">{hint}</p>
	{/if}

	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.75rem">
		<button type="button" class="ldk-btn" disabled={busy} data-testid="dxf-connect-mock" onclick={() => connectMock()}
			>Connect (mock)</button
		>
		<button
			type="button"
			class="ldk-btn secondary"
			disabled={busy}
			data-testid="dxf-disconnect"
			onclick={() => disconnectRtc()}>Disconnect</button
		>
		<button type="button" class="ldk-btn secondary" disabled={busy} onclick={() => refreshRtc()}>Refresh RTC</button>
	</div>
	<p class="ldk-muted" data-testid="dxf-rtc-state">
		RTC: <strong>{rtcState}</strong>
		{#if dxfLineCount != null}
			· DXF segments: <strong>{dxfLineCount}</strong>
		{/if}
	</p>

	<h2 style="font-size:1.05rem;margin-top:1.25rem">Parse DXF</h2>
	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.75rem">
		<button
			type="button"
			class="ldk-btn"
			disabled={busy}
			data-testid="dxf-load-demo-static"
			onclick={() => loadDemoFromStatic()}>Load SCANLAB demo (from site)</button
		>
		<button
			type="button"
			class="ldk-btn secondary"
			disabled={busy}
			data-testid="dxf-load-demo-server"
			onclick={() => loadDemoFromServer()}>Load via server file</button
		>
	</div>
	<p class="ldk-muted" style="font-size:0.85rem">
		<strong>From site</strong> works with the static copy under <code>{base}/demo/dxf/</code> (GitHub Pages).
		<strong>Server file</strong> needs the repo’s <code>demo/dxf/</code> on disk or <code>LASERDESK_DEMO_DXF</code>.
	</p>

	{#if job}
		<p class="ldk-muted" data-testid="dxf-entity-count">
			<strong>{job.line_count}</strong> LINE entities · source <code>{job.source_name}</code>
		</p>

		{#if layout}
			<div class="ldk-dxf-preview" data-testid="dxf-svg-wrap">
				<svg
					viewBox="{layout.minX} {layout.minY} {layout.w} {layout.h}"
					width="100%"
					height="320"
					preserveAspectRatio="xMidYMid meet"
					role="img"
					aria-label="DXF line preview"
				>
					{#each job.entities as e, i (e.index)}
						<line
							x1={e.x0}
							y1={layout.flip(e.y0)}
							x2={e.x1}
							y2={layout.flip(e.y1)}
							stroke={selectedIndex === i ? '#0a5' : '#246'}
							stroke-width={Math.max(layout.w, layout.h) * 0.002}
							vector-effect="non-scaling-stroke"
						/>
					{/each}
				</svg>
			</div>
		{/if}

		<div style="overflow:auto;max-height:14rem;border:1px solid #d8dee6;border-radius:6px">
			<table class="ldk-table" data-testid="dxf-entity-table">
				<thead>
					<tr>
						<th>#</th>
						<th>Layer</th>
						<th>Length</th>
						<th>From</th>
						<th>To</th>
					</tr>
				</thead>
				<tbody>
					{#each job.entities as e, i (e.index)}
						<tr
							class:ldk-row-sel={selectedIndex === i}
							onclick={() => (selectedIndex = selectedIndex === i ? null : i)}
							style="cursor:pointer"
						>
							<td>{e.index}</td>
							<td>{e.layer}</td>
							<td>{e.length.toFixed(3)}</td>
							<td>({e.x0.toFixed(2)}, {e.y0.toFixed(2)})</td>
							<td>({e.x1.toFixed(2)}, {e.y1.toFixed(2)})</td>
						</tr>
					{/each}
				</tbody>
			</table>
		</div>

		<h2 style="font-size:1.05rem;margin-top:1.25rem">RTC</h2>
		<div class="ldk-row" style="flex-wrap:wrap">
			<button
				type="button"
				class="ldk-btn"
				disabled={busy || !jobId}
				data-testid="dxf-load-rtc"
				onclick={() => loadIntoRtc()}>Load into RTC</button
			>
			<button
				type="button"
				class="ldk-btn"
				disabled={busy || !jobId || running || !loaded}
				data-testid="dxf-start"
				onclick={() => startRun()}>Start</button
			>
			<button
				type="button"
				class="ldk-btn danger"
				disabled={busy || !jobId || !running}
				data-testid="dxf-stop"
				onclick={() => stopRun()}>Stop</button
			>
		</div>
	{/if}
</article>

<style>
	.ldk-dxf-preview {
		border: 1px solid #d8dee6;
		border-radius: 6px;
		background: #fafbfc;
		margin-bottom: 0.75rem;
	}
	.ldk-table {
		width: 100%;
		border-collapse: collapse;
		font-size: 0.85rem;
	}
	.ldk-table th,
	.ldk-table td {
		padding: 0.35rem 0.5rem;
		text-align: left;
		border-bottom: 1px solid #e8ecf0;
	}
	.ldk-table th {
		background: #f4f6f8;
		font-weight: 600;
	}
	:global(tr.ldk-row-sel) {
		background: #e8f5ee;
	}
</style>
