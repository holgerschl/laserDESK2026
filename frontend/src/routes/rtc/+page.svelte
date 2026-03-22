<script lang="ts">
	import { base } from '$app/paths';
	import { onDestroy, onMount } from 'svelte';
	import * as api from '$lib/api/laserdesk';
	import { LASERDESK_RTC_CHANNEL, type RtcChannelMessage } from '$lib/laser/rtcChannel';

	let logLines = $state<string[]>([]);
	let channel: BroadcastChannel | null = null;
	let pollTimer: ReturnType<typeof setInterval> | null = null;
	let statusText = $state('—');
	let healthText = $state('—');

	function appendLog(line: string, ts: number) {
		const row = `[${new Date(ts).toISOString()}] ${line}`;
		logLines = [...logLines.slice(-250), row];
	}

	onMount(() => {
		try {
			channel = new BroadcastChannel(LASERDESK_RTC_CHANNEL);
			channel.onmessage = (ev: MessageEvent<RtcChannelMessage>) => {
				const d = ev.data;
				if (d?.type === 'log') appendLog(d.line, d.ts);
			};
		} catch {
			channel = null;
		}
		void refresh();
		pollTimer = setInterval(refresh, 2500);
	});

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
	Separate monitor window: RTC/health polling here. The <strong>activity log</strong> only shows lines sent from
	other tabs on this origin (workflow or DXF demo) via <code>BroadcastChannel</code>
	<code>{LASERDESK_RTC_CHANNEL}</code> — not raw UDP traffic from the board. Open <code>/workflow</code> or
	<code>/dxf</code> alongside this tab.
</p>

<div class="ldk-row" style="margin-bottom:1rem">
	<button type="button" class="ldk-btn secondary" data-testid="rtc-refresh" onclick={() => refresh()}
		>Refresh now</button
	>
	<a href="{base}/usage" data-testid="rtc-back-home">Usage</a>
	<a href="{base}/workflow" data-testid="rtc-back-workflow">Workflow</a>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.5rem;font-size:1rem">Health</h2>
	<pre class="ldk-pre" data-testid="rtc-page-health">{healthText}</pre>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.5rem;font-size:1rem">RTC status</h2>
	<pre class="ldk-pre" data-testid="rtc-page-status">{statusText}</pre>
</div>

<div class="ldk-card">
	<h2 style="margin:0 0 0.5rem;font-size:1rem">Activity log</h2>
	<pre class="ldk-pre" data-testid="rtc-channel-log">{logLines.join('\n') || '(no messages yet)'}</pre>
</div>
