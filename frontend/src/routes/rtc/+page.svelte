<script lang="ts">
	import { base } from '$app/paths';
	import { onDestroy, onMount } from 'svelte';
	import RtcConnectionPanel from '$lib/components/RtcConnectionPanel.svelte';
	import * as api from '$lib/api/laserdesk';
	import { LASERDESK_RTC_CHANNEL, type RtcChannelMessage } from '$lib/laser/rtcChannel';

	let logLines = $state<string[]>([]);
	let channel = $state<BroadcastChannel | null>(null);
	let pollTimer: ReturnType<typeof setInterval> | null = null;
	let statusText = $state('—');
	let healthText = $state('—');
	let rifLogText = $state('—');
	/** Same tab’s origin + base path; workflow/dxf must match exactly (incl. localhost vs 127.0.0.1). */
	let thisSiteLabel = $state('');

	function appendLog(line: string, ts: number) {
		const row = `[${new Date(ts).toISOString()}] ${line}`;
		logLines = [...logLines.slice(-250), row];
	}

	onMount(() => {
		thisSiteLabel = `${window.location.origin}${base}`;
		try {
			channel = new BroadcastChannel(LASERDESK_RTC_CHANNEL);
			channel.onmessage = (ev: MessageEvent<RtcChannelMessage>) => {
				const d = ev.data;
				if (d?.type === 'log') appendLog(d.line, d.ts);
			};
		} catch {
			channel = null;
			appendLog(
				'BroadcastChannel not available in this browser/context — cross-tab activity lines will not appear.',
				Date.now()
			);
		}
		appendLog(`This window: ${thisSiteLabel} — workflow/DXF must use this exact origin (not localhost on one tab and 127.0.0.1 on the other).`, Date.now());
		appendLog(
			'Listening — the log does not keep history. Leave this tab open, then use /workflow or /dxf in another tab here, or repeat Connect/Start so new lines appear.',
			Date.now()
		);
		void refresh();
		pollTimer = setInterval(refresh, 2500);
	});

	function pingBroadcastChannel() {
		if (!channel) return;
		const ts = Date.now();
		const line = 'BroadcastChannel ping from RTC tab (same-tab echo checks your browser).';
		try {
			channel.postMessage({ type: 'log', line, ts });
		} catch (e) {
			appendLog(e instanceof Error ? e.message : String(e), Date.now());
		}
	}

	function appendLocalTestLine() {
		appendLog('Local test line (not sent on BroadcastChannel — if you see only this, the panel works).', Date.now());
	}

	onDestroy(() => {
		if (pollTimer) clearInterval(pollTimer);
		try {
			channel?.close();
		} catch {
			/* ignore */
		}
	});

	async function refresh() {
		try {
			const [h, s] = await Promise.all([api.getHealth(), api.getRtcStatus()]);
			healthText = JSON.stringify(h, null, 2);
			statusText = JSON.stringify(s, null, 2);
		} catch (e) {
			statusText = e instanceof Error ? e.message : String(e);
		}
	}
</script>

<svelte:head>
	<title>RTC · laserDESK 2026</title>
</svelte:head>

<p class="ldk-muted" style="margin-top:0">
	Separate monitor window: RTC/health polling here. <strong>RIF command log</strong> (below) lists outbound Remote
	Interface payloads the backend sent (seq, mnemonic, <code>u32</code> words; <code>f=</code> for IEEE doubles on speed
	telegrams). The <strong>activity log</strong> only shows short text lines from
	other tabs on this <strong>same origin</strong> (workflow or DXF) via <code>BroadcastChannel</code>
	<code>{LASERDESK_RTC_CHANNEL}</code> — not raw UDP telegrams from the board. Keep this tab on <strong>/rtc</strong> and open
	<code>/workflow</code> or <code>/dxf</code> in a <strong>second tab</strong> (two <code>/rtc</code> tabs only listen — nothing
	is sent from here). Messages are <strong>not</strong> buffered,
	so actions you already did before opening RTC will not show up.
	<strong>localhost</strong> and <strong>127.0.0.1</strong> count as different sites — use the same host in both tabs.
</p>
{#if thisSiteLabel}
	<p class="ldk-muted" style="margin:0 0 0.45rem;font-size:0.82rem" data-testid="rtc-origin-label">
		<strong>This URL:</strong> <code>{thisSiteLabel}</code>
	</p>
{/if}

<RtcConnectionPanel onSessionChanged={() => refresh()} />

<div class="ldk-row" style="margin-bottom:1rem">
	<button type="button" class="ldk-btn secondary" data-testid="rtc-refresh" onclick={() => refresh()}
		>Refresh now</button
	>
	<button
		type="button"
		class="ldk-btn secondary"
		data-testid="rtc-bc-ping"
		disabled={!channel}
		onclick={() => pingBroadcastChannel()}>Ping BroadcastChannel</button
	>
	<button type="button" class="ldk-btn secondary" data-testid="rtc-local-test" onclick={() => appendLocalTestLine()}
		>Local test line</button
	>
	<a href="{base}/usage" data-testid="rtc-back-home">Usage</a>
	<a href="{base}/workflow" data-testid="rtc-back-workflow">Workflow</a>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.35rem;font-size:0.95rem">Health</h2>
	<pre class="ldk-pre" data-testid="rtc-page-health">{healthText}</pre>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.5rem;font-size:1rem">RTC status</h2>
	<pre class="ldk-pre" data-testid="rtc-page-status">{statusText}</pre>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.35rem;font-size:0.95rem">RIF command log</h2>
	<p class="ldk-muted" style="margin:0 0 0.4rem;font-size:0.82rem">
		Polled with status every 2.5s. Ethernet only accumulates lines; mock stays empty. New connect clears the log.
	</p>
	<pre class="ldk-pre ldk-rif-log" data-testid="rtc-rif-log">{rifLogText}</pre>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.35rem;font-size:0.95rem">Activity log</h2>
	<pre class="ldk-pre" data-testid="rtc-channel-log">{logLines.join('\n')}</pre>
</div>
