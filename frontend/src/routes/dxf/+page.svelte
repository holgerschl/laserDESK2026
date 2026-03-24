<script lang="ts">
	import { base } from '$app/paths';
	import RtcConnectionPanel from '$lib/components/RtcConnectionPanel.svelte';
	import * as api from '$lib/api/laserdesk';
	import type { DxfJobEntity, DxfJobResponse } from '$lib/api/laserdesk';
	import { postRtcLog } from '$lib/laser/rtcChannel';
	import { fmtMmLabel, mmTicks, niceMmStep } from '$lib/scene/mmAxes';
	import { onMount, tick } from 'svelte';

	function rtcLog(line: string) {
		postRtcLog(line);
	}

	onMount(() => {
		rtcLog(
			`DXF demo: page ready (${typeof window !== 'undefined' ? window.location.origin : ''}${base}) — further lines only after buttons (Connect, Load demo, Load into RTC, Start/Stop), not from viewing the page alone.`
		);

		let lastFocusLog = 0;
		const onVis = () => {
			if (document.visibilityState !== 'visible') return;
			const now = Date.now();
			if (now - lastFocusLog < 2500) return;
			lastFocusLog = now;
			rtcLog('DXF demo: tab focused — use Connect (mock) or Load demo if the log is still empty.');
		};
		document.addEventListener('visibilitychange', onVis);

		return () => document.removeEventListener('visibilitychange', onVis);
	});

	let busy = $state(false);
	let err = $state<string | null>(null);
	let hint = $state<string | null>(null);
	let jobId = $state<string | null>(null);
	let job = $state<DxfJobResponse | null>(null);
	let selectedIndex = $state<number | null>(null);
	let rtcState = $state<string>('—');
	let dxfLineCount = $state<number | null>(null);
	let runRepeatCount = $state(1);

	function toggleEntitySelection(i: number) {
		selectedIndex = selectedIndex === i ? null : i;
	}

	$effect(() => {
		const idx = selectedIndex;
		if (idx === null || typeof document === 'undefined') return;
		void tick().then(() => {
			document
				.querySelector<HTMLElement>(`[data-dxf-entity-row="${idx}"]`)
				?.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
		});
	});

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
		return {
			minX: minX - padX,
			minY: minY - padY,
			maxX: maxX + padX,
			maxY: maxY + padY,
			w,
			h,
			flip,
			worldMinX: minX,
			worldMaxX: maxX,
			worldMinY: minY,
			worldMaxY: maxY,
			viewWorldMinY: minY - padY,
			viewWorldMaxY: maxY + padY
		};
	}

	$effect(() => {
		void refreshRtc();
	});

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
			rtcLog(`DXF demo: parsed ${job.line_count} LINE entities (static demo file)`);
		});
	}

	async function loadDemoFromServer() {
		await withBusy(async () => {
			const r = await api.postJobsDxfDemo();
			jobId = r.job_id;
			job = await api.getJobsDxf(r.job_id);
			selectedIndex = null;
			hint = `Parsed ${job.line_count} LINE entities (server file).`;
			rtcLog(`DXF demo: parsed ${job.line_count} LINE entities (server demo)`);
		});
	}

	async function loadIntoRtc() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfLoad(id);
			await refreshRtc();
			hint = 'DXF job loaded into RTC session.';
			rtcLog(
				'DXF demo: POST /jobs/dxf/…/load — ethernet: R_DC_CONFIG_LIST (if dxf_rif_list_upload) + R_DC_GET_INPUT_POINTER + R_LC_JUMP/MARK telegrams; mock: internal load'
			);
		});
	}

	async function startRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfRun(id, runRepeatCount);
			await refreshRtc();
			hint = 'Execution started.';
			rtcLog(
				`DXF demo: POST /jobs/dxf/…/run?repeat_count=${runRepeatCount} — ethernet: R_DC_SET_MAX_COUNT + R_DC_EXECUTE_LIST_POS; mock: unchanged`
			);
		});
	}

	async function stopRun() {
		const id = jobId;
		if (!id) return;
		await withBusy(async () => {
			await api.postJobsDxfStop(id);
			await refreshRtc();
			hint = 'Execution stopped.';
			rtcLog('DXF demo: POST /jobs/dxf/…/stop — ethernet: R_DC_STOP_EXECUTION; mock: state → loaded');
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
		Load the reference <code>SCANLABLogo.dxf</code> (LINE entities), preview geometry, connect mock or real RTC, load the
		job, then start/stop execution. Backend API:
		<code>/api/v1/jobs/dxf</code>.
	</p>

	{#if err}
		<p class="ldk-error" role="alert" data-testid="dxf-page-error">{err}</p>
	{/if}
	{#if hint}
		<p class="ldk-success" role="status" data-testid="dxf-page-hint">{hint}</p>
	{/if}

	<RtcConnectionPanel
		onSessionChanged={() => {
			void refreshRtc();
		}}
		onAfterConnect={({ mode: m, host }) => {
			if (m === 'mock') {
				rtcLog('DXF demo: POST /rtc/connect (mock — no UDP telegrams)');
			} else {
				rtcLog(`DXF demo: POST /rtc/connect (ethernet ${host ?? ''})`);
			}
		}}
		onAfterDisconnect={() => {
			jobId = null;
			job = null;
			void refreshRtc();
			rtcLog('DXF demo: POST /rtc/disconnect');
		}}
	/>
	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.45rem">
		<button type="button" class="ldk-btn secondary" disabled={busy} onclick={() => refreshRtc()}>Refresh RTC</button>
	</div>
	<p class="ldk-muted" data-testid="dxf-rtc-state">
		RTC: <strong>{rtcState}</strong>
		{#if dxfLineCount != null}
			· DXF segments: <strong>{dxfLineCount}</strong>
		{/if}
	</p>
	<p class="ldk-muted" style="font-size:0.85rem">
		<a href="{base}/rtc">RTC activity log</a>: same host as this page (not <code>localhost</code> mixed with
		<code>127.0.0.1</code>). Lines are sent when you use the buttons above (Connect, Load demo, …), not merely by
		opening this tab — switching back to this tab emits at most a short “tab focused” hint (throttled).
	</p>

	<h2 style="font-size:0.98rem;margin-top:0.85rem">Parse DXF</h2>
	<div class="ldk-row" style="flex-wrap:wrap;margin-bottom:0.45rem">
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
			{@const previewStroke = Math.max(Math.max(layout.w, layout.h) * 0.004, 0.09)}
			{@const axisStroke = Math.max(previewStroke * 0.7, 0.1)}
			{@const labelSize = Math.max(Math.max(layout.w, layout.h) * 0.034, 0.65)}
			{@const y0 = layout.flip(0)}
			{@const hitStroke = Math.max(previewStroke * 2.6, Math.max(layout.w, layout.h) * 0.0055)}
			{@const tickStepX = niceMmStep(layout.w)}
			{@const tickStepY = niceMmStep(layout.h)}
			{@const xs = mmTicks(layout.minX, layout.maxX, tickStepX)}
			{@const ysW = mmTicks(layout.viewWorldMinY, layout.viewWorldMaxY, tickStepY)}
			{@const bottomY = layout.minY + layout.h}
			{@const tickLen = Math.max(Math.max(layout.w, layout.h) * 0.014, 0.22)}
			{@const tickFont = Math.max(Math.max(layout.w, layout.h) * 0.026, 0.5)}
			{@const xAxisInView = y0 >= layout.minY && y0 <= layout.minY + layout.h}
			{@const yAxisInView = 0 >= layout.minX && 0 <= layout.maxX}
			<div class="ldk-dxf-preview" data-testid="dxf-svg-wrap">
				<svg
					viewBox="{layout.minX} {layout.minY} {layout.w} {layout.h}"
					width="100%"
					height="320"
					preserveAspectRatio="xMidYMid meet"
					overflow="visible"
					role="img"
					aria-label="DXF line preview with coordinate axes"
				>
					<g class="dxf-coords" pointer-events="none" aria-hidden="true">
						<rect
							x={layout.minX}
							y={layout.minY}
							width={layout.w}
							height={layout.h}
							fill="none"
							stroke="#cbd5e1"
							stroke-width={axisStroke}
						/>
						<!-- mm ticks along bottom (X) and left (Y); labels assume drawing units = mm -->
						{#each xs as xt (xt)}
							{#if xt >= layout.minX - 1e-9 && xt <= layout.maxX + 1e-9}
								<line
									x1={xt}
									y1={bottomY}
									x2={xt}
									y2={bottomY + tickLen}
									stroke="#94a3b8"
									stroke-width={axisStroke}
								/>
								<text
									x={xt}
									y={bottomY + tickLen + tickFont * 0.92}
									fill="#64748b"
									font-size={tickFont}
									font-family="system-ui, Segoe UI, sans-serif"
									text-anchor="middle">{fmtMmLabel(xt)}</text>
							{/if}
						{/each}
						{#each ysW as yt (yt)}
							{@const ys = layout.flip(yt)}
							{#if ys >= layout.minY - 1e-9 && ys <= bottomY + 1e-9}
								<line
									x1={layout.minX - tickLen}
									y1={ys}
									x2={layout.minX}
									y2={ys}
									stroke="#94a3b8"
									stroke-width={axisStroke}
								/>
								<text
									x={layout.minX - tickLen - tickFont * 0.25}
									y={ys + tickFont * 0.32}
									fill="#64748b"
									font-size={tickFont}
									font-family="system-ui, Segoe UI, sans-serif"
									text-anchor="end">{fmtMmLabel(yt)}</text>
							{/if}
						{/each}
						{#if xAxisInView}
							<line
								x1={layout.minX}
								y1={y0}
								x2={layout.maxX}
								y2={y0}
								stroke="#94a3b8"
								stroke-width={axisStroke}
								stroke-dasharray={`${axisStroke * 3} ${axisStroke * 2}`}
							/>
						{/if}
						{#if yAxisInView}
							<line
								x1="0"
								y1={layout.minY}
								x2="0"
								y2={layout.minY + layout.h}
								stroke="#94a3b8"
								stroke-width={axisStroke}
								stroke-dasharray={`${axisStroke * 3} ${axisStroke * 2}`}
							/>
						{/if}
						{#if xAxisInView && yAxisInView}
							<circle cx="0" cy={y0} r={Math.max(axisStroke * 2.2, 0.35)} fill="#64748b" />
						{/if}
						{#if xAxisInView}
							<text
								x={layout.maxX - labelSize * 0.15}
								y={y0 + labelSize * 0.85}
								fill="#64748b"
								font-size={labelSize}
								font-family="system-ui, Segoe UI, sans-serif"
								text-anchor="end"
								font-weight="600">+X</text>
						{/if}
						{#if yAxisInView}
							<text
								x={labelSize * 0.35}
								y={layout.minY + labelSize * 0.95}
								fill="#64748b"
								font-size={labelSize}
								font-family="system-ui, Segoe UI, sans-serif"
								text-anchor="start"
								font-weight="600">+Y</text>
						{/if}
					</g>
					{#each job.entities as e, i (e.index)}
						<g class="dxf-entity-seg">
							<line
								x1={e.x0}
								y1={layout.flip(e.y0)}
								x2={e.x1}
								y2={layout.flip(e.y1)}
								fill="none"
								pointer-events="none"
								stroke={selectedIndex === i ? '#059669' : '#246'}
								stroke-width={selectedIndex === i ? previewStroke * 1.22 : previewStroke}
								stroke-linecap="round"
								stroke-linejoin="round"
							/>
							<line
								x1={e.x0}
								y1={layout.flip(e.y0)}
								x2={e.x1}
								y2={layout.flip(e.y1)}
								fill="none"
								stroke="transparent"
								stroke-width={hitStroke}
								stroke-linecap="round"
								class="dxf-entity-hit"
								aria-hidden="true"
								onclick={() => toggleEntitySelection(i)}
							/>
						</g>
					{/each}
				</svg>
				<p class="ldk-muted dxf-coords-caption">
					Axes and tick labels use <strong>DXF drawing units as millimetres</strong> (demo assumption). Origin at
					(0, 0); <strong>+Y</strong> points up on screen (vertical flip vs CAD).
				</p>
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
							data-dxf-entity-row={i}
							class:ldk-row-sel={selectedIndex === i}
							onclick={() => toggleEntitySelection(i)}
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

		<h2 style="font-size:0.98rem;margin-top:0.85rem">RTC</h2>
		<div class="ldk-field" style="max-width:11rem;margin-bottom:0.45rem">
			<label for="dxf-run-repeats">Repeats</label>
			<input
				id="dxf-run-repeats"
				type="number"
				class="ldk-input"
				min="1"
				max="1000000"
				bind:value={runRepeatCount}
				disabled={busy || running}
			/>
		</div>
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
		min-height: 200px;
		overflow: visible;
		padding-bottom: 1.35rem;
	}
	.ldk-dxf-preview :global(svg) {
		display: block;
		overflow: visible;
	}
	:global(.dxf-entity-hit) {
		cursor: pointer;
	}
	.dxf-coords-caption {
		margin: 0.22rem 0 0;
		font-size: 0.78rem;
		line-height: 1.3;
		padding: 0 0.2rem;
	}
	.ldk-table {
		width: 100%;
		border-collapse: collapse;
		font-size: 0.82rem;
	}
	.ldk-table th,
	.ldk-table td {
		padding: 0.25rem 0.4rem;
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
