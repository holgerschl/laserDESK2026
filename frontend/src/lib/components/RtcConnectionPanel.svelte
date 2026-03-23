<script lang="ts">
	import * as api from '$lib/api/laserdesk';
	import type { DiscoveredRtcHost } from '$lib/api/laserdesk';

	type PanelMode = 'mock' | 'ethernet';

	interface Props {
		/** After connect/disconnect refresh parent status if needed */
		onSessionChanged?: () => void | Promise<void>;
		/** e.g. clear local job state when session ends */
		onAfterDisconnect?: () => void | Promise<void>;
		/** Parent status log / hints */
		onAfterConnect?: (info: { mode: PanelMode; host?: string }) => void;
		/** Parent guard (e.g. workflow: block reconnect while execution running) */
		connectDisabled?: boolean;
		disconnectDisabled?: boolean;
	}
	let {
		onSessionChanged,
		onAfterDisconnect,
		onAfterConnect,
		connectDisabled = false,
		disconnectDisabled = false
	}: Props = $props();

	let mode = $state<PanelMode>('mock');
	let ethHost = $state('192.168.1.50');
	let ethPort = $state(5020);
	let tgmFormat = $state(0);
	let busy = $state(false);
	let err = $state<string | null>(null);
	let hint = $state<string | null>(null);

	let discoverOpen = $state(false);
	let discoverBase = $state('192.168.1.0');
	let discoverMask = $state('255.255.255.0');
	let discoverPort = $state(5020);
	let discoverTimeout = $state(120);
	let discoverMaxHosts = $state(512);
	let discoverBusy = $state(false);
	let discoverErr = $state<string | null>(null);
	let discoverResults = $state<DiscoveredRtcHost[]>([]);
	let discoverMeta = $state<{ scanned: number; elapsed_ms: number } | null>(null);

	function clearMsgs() {
		err = null;
		hint = null;
	}

	async function notifyChanged() {
		await onSessionChanged?.();
	}

	async function doConnect() {
		clearMsgs();
		busy = true;
		try {
			if (mode === 'mock') {
				await api.postRtcConnect({ mode: 'mock' });
				hint = 'Connected (mock RTC).';
				onAfterConnect?.({ mode: 'mock' });
			} else {
				const host = ethHost.trim();
				if (!host) throw new Error('Enter the RTC board IP or hostname.');
				await api.postRtcConnect({
					mode: 'ethernet',
					host,
					port: Number(ethPort),
					tgm_format: Number(tgmFormat)
				});
				hint = `Connected to real RTC at ${host}:${ethPort} (UDP Remote Interface).`;
				onAfterConnect?.({ mode: 'ethernet', host });
			}
			await notifyChanged();
		} catch (e) {
			err = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	async function doDisconnect() {
		clearMsgs();
		busy = true;
		try {
			await api.postRtcDisconnect();
			hint = 'Disconnected.';
			await onAfterDisconnect?.();
			await notifyChanged();
		} catch (e) {
			err = e instanceof Error ? e.message : String(e);
		} finally {
			busy = false;
		}
	}

	function openDiscover() {
		discoverErr = null;
		discoverResults = [];
		discoverMeta = null;
		discoverPort = ethPort;
		discoverOpen = true;
	}

	function closeDiscover() {
		discoverOpen = false;
	}

	async function runDiscover() {
		discoverErr = null;
		discoverResults = [];
		discoverMeta = null;
		discoverBusy = true;
		try {
			const r = await api.postRtcDiscover({
				base_ip: discoverBase.trim(),
				netmask: discoverMask.trim(),
				port: Number(discoverPort),
				timeout_ms: Number(discoverTimeout),
				max_hosts: Number(discoverMaxHosts),
				tgm_format: Number(tgmFormat)
			});
			discoverResults = r.hosts;
			discoverMeta = { scanned: r.scanned, elapsed_ms: r.elapsed_ms };
			if (r.hosts.length === 0) {
				discoverErr = `No RTC answered R_DC_GET_STATUS on UDP port ${discoverPort} (${r.scanned} addresses, ${r.elapsed_ms} ms). Check subnet, firewall, and board Remote Interface mode.`;
			}
		} catch (e) {
			discoverErr = e instanceof Error ? e.message : String(e);
		} finally {
			discoverBusy = false;
		}
	}

	function pickHost(h: DiscoveredRtcHost) {
		ethHost = h.ip;
		ethPort = Number(discoverPort);
		mode = 'ethernet';
		closeDiscover();
		hint = `Set board address to ${h.ip} (status ${h.remote_status}, pos ${h.remote_pos}). Click Connect (real RTC).`;
	}
</script>

<div class="ldk-card ldk-rtc-panel" data-testid="rtc-connection-panel">
	<h2 class="ldk-rtc-panel-title">RTC session</h2>
	<p class="ldk-muted ldk-rtc-panel-lead">
		<strong>Mock</strong> = no hardware. <strong>Real RTC</strong> = UDP Remote Interface to the card (same
		<code>R_DC_GET_STATUS</code> handshake as <code>POST /rtc/connect</code> with <code>mode: ethernet</code>).
	</p>

	<fieldset class="ldk-rtc-mode-fieldset">
		<legend class="ldk-sr-only">Connection mode</legend>
		<div class="ldk-row ldk-rtc-mode-row">
			<label class="ldk-radio-label">
				<input type="radio" name="rtc-mode" value="mock" bind:group={mode} disabled={busy || connectDisabled} />
				Mock
			</label>
			<label class="ldk-radio-label">
				<input type="radio" name="rtc-mode" value="ethernet" bind:group={mode} disabled={busy} />
				Real RTC (Ethernet / UDP)
			</label>
		</div>
	</fieldset>

	{#if mode === 'ethernet'}
		<div class="ldk-field">
			<label for="rtc-eth-host">Board IP or hostname</label>
			<input
				id="rtc-eth-host"
				type="text"
				class="ldk-input"
				autocomplete="off"
				data-testid="rtc-eth-host"
				bind:value={ethHost}
				disabled={busy || connectDisabled}
			/>
		</div>
		<div class="ldk-row ldk-rtc-eth-row">
			<div class="ldk-field ldk-field-grow">
				<label for="rtc-eth-port">UDP port</label>
				<input
					id="rtc-eth-port"
					type="number"
					class="ldk-input"
					data-testid="rtc-eth-port"
					bind:value={ethPort}
					disabled={busy || connectDisabled}
					min="1"
					max="65535"
				/>
			</div>
			<div class="ldk-field ldk-field-grow">
				<label for="rtc-tgm-format">TGM format (RAW)</label>
				<input
					id="rtc-tgm-format"
					type="number"
					class="ldk-input"
					data-testid="rtc-tgm-format"
					bind:value={tgmFormat}
					disabled={busy}
					min="0"
				/>
			</div>
		</div>
		<div class="ldk-row" style="flex-wrap:wrap;gap:0.5rem;margin-bottom:0.75rem">
			<button type="button" class="ldk-btn secondary" disabled={busy || connectDisabled} onclick={openDiscover}
				>Discover RTC…</button
			>
		</div>
	{/if}

	{#if err}
		<p class="ldk-error" role="alert" data-testid="rtc-panel-error">{err}</p>
	{/if}
	{#if hint}
		<p class="ldk-success" role="status" data-testid="rtc-panel-hint">{hint}</p>
	{/if}

	<div class="ldk-row" style="flex-wrap:wrap;gap:0.5rem">
		<button
			type="button"
			class="ldk-btn"
			disabled={busy || connectDisabled}
			data-testid="connect-mock"
			onclick={doConnect}
		>
			{mode === 'mock' ? 'Connect (mock)' : 'Connect (real RTC)'}
		</button>
		<button
			type="button"
			class="ldk-btn secondary"
			disabled={busy || disconnectDisabled}
			data-testid="disconnect-rtc"
			onclick={doDisconnect}>Disconnect</button
		>
	</div>
</div>

{#if discoverOpen}
	<div
		class="ldk-modal-backdrop"
		role="presentation"
		onclick={closeDiscover}
		onkeydown={(e) => e.key === 'Escape' && closeDiscover()}
	>
		<!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
		<div
			class="ldk-modal"
			role="dialog"
			tabindex="-1"
			aria-modal="true"
			aria-labelledby="rtc-discover-title"
			onclick={(e) => e.stopPropagation()}
			onkeydown={(e) => e.stopPropagation()}
		>
			<h2 id="rtc-discover-title" class="ldk-modal-title">Discover RTC on subnet</h2>
			<p class="ldk-muted" style="margin-top:0;font-size:0.88rem">
				Enter any IPv4 on the LAN segment and the netmask. The backend probes each host with
				<code>R_DC_GET_STATUS</code> on the UDP port (default 5020). Large subnets are capped by “max hosts”.
			</p>
			<div class="ldk-field">
				<label for="disc-base">Base IP (any address on the subnet)</label>
				<input id="disc-base" class="ldk-input" bind:value={discoverBase} disabled={discoverBusy} />
			</div>
			<div class="ldk-field">
				<label for="disc-mask">Subnet mask</label>
				<input id="disc-mask" class="ldk-input" bind:value={discoverMask} disabled={discoverBusy} />
			</div>
			<div class="ldk-row" style="gap:0.75rem;flex-wrap:wrap">
				<div class="ldk-field ldk-field-grow">
					<label for="disc-port">UDP port</label>
					<input
						id="disc-port"
						type="number"
						class="ldk-input"
						bind:value={discoverPort}
						disabled={discoverBusy}
						min="1"
						max="65535"
					/>
				</div>
				<div class="ldk-field ldk-field-grow">
					<label for="disc-to">Timeout ms / host</label>
					<input
						id="disc-to"
						type="number"
						class="ldk-input"
						bind:value={discoverTimeout}
						disabled={discoverBusy}
						min="30"
						max="2000"
					/>
				</div>
				<div class="ldk-field ldk-field-grow">
					<label for="disc-max">Max hosts</label>
					<input
						id="disc-max"
						type="number"
						class="ldk-input"
						bind:value={discoverMaxHosts}
						disabled={discoverBusy}
						min="1"
						max="4096"
					/>
				</div>
			</div>
			{#if discoverErr}
				<p class="ldk-error" role="alert">{discoverErr}</p>
			{/if}
			{#if discoverMeta}
				<p class="ldk-muted" style="font-size:0.85rem">
					Scanned {discoverMeta.scanned} addresses in {discoverMeta.elapsed_ms} ms.
				</p>
			{/if}
			{#if discoverResults.length > 0}
				<ul class="ldk-discover-list">
					{#each discoverResults as h (h.ip)}
						<li>
							<button type="button" class="ldk-btn secondary ldk-discover-pick" onclick={() => pickHost(h)}>
								Use {h.ip}
							</button>
							<span class="ldk-muted" style="font-size:0.85rem">
								status {h.remote_status}, pos {h.remote_pos}
							</span>
						</li>
					{/each}
				</ul>
			{/if}
			<div class="ldk-modal-actions">
				<button type="button" class="ldk-btn" disabled={discoverBusy} onclick={runDiscover}>Scan subnet</button>
				<button type="button" class="ldk-btn secondary" disabled={discoverBusy} onclick={closeDiscover}
					>Close</button
				>
			</div>
		</div>
	</div>
{/if}

<style>
	.ldk-rtc-panel-title {
		margin: 0 0 0.35rem;
		font-size: 1.05rem;
	}
	.ldk-rtc-panel-lead {
		margin: 0 0 0.75rem;
		font-size: 0.88rem;
	}
	.ldk-rtc-mode-fieldset {
		border: none;
		padding: 0;
		margin: 0 0 0.75rem;
	}
	.ldk-rtc-mode-row {
		gap: 1.25rem;
	}
	.ldk-radio-label {
		display: inline-flex;
		align-items: center;
		gap: 0.35rem;
		cursor: pointer;
		font-size: 0.95rem;
	}
	.ldk-sr-only {
		position: absolute;
		width: 1px;
		height: 1px;
		padding: 0;
		margin: -1px;
		overflow: hidden;
		clip: rect(0, 0, 0, 0);
		white-space: nowrap;
		border: 0;
	}
	.ldk-field {
		margin-bottom: 0.65rem;
	}
	.ldk-field-grow {
		flex: 1;
		min-width: 6rem;
	}
	.ldk-field label {
		display: block;
		font-size: 0.85rem;
		margin-bottom: 0.2rem;
		color: #3d4d5c;
	}
	.ldk-input {
		width: 100%;
		box-sizing: border-box;
		font: inherit;
		padding: 0.35rem 0.5rem;
		border: 1px solid #c5ced6;
		border-radius: 4px;
	}
	.ldk-modal-backdrop {
		position: fixed;
		inset: 0;
		background: rgba(15, 20, 25, 0.45);
		display: flex;
		align-items: center;
		justify-content: center;
		z-index: 80;
		padding: 1rem;
	}
	.ldk-modal {
		background: #fff;
		border-radius: 8px;
		max-width: 32rem;
		width: 100%;
		padding: 1rem 1.25rem 1.25rem;
		box-shadow: 0 8px 32px rgba(0, 0, 0, 0.18);
		max-height: 90vh;
		overflow: auto;
	}
	.ldk-modal-title {
		margin: 0 0 0.5rem;
		font-size: 1.1rem;
	}
	.ldk-modal-actions {
		display: flex;
		flex-wrap: wrap;
		gap: 0.5rem;
		margin-top: 1rem;
	}
	.ldk-discover-list {
		list-style: none;
		padding: 0;
		margin: 0.5rem 0 0;
	}
	.ldk-discover-list li {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 0.5rem;
		margin-bottom: 0.35rem;
	}
	.ldk-discover-pick {
		font-size: 0.88rem;
	}
</style>
