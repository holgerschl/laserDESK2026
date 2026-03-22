/** Cross-window messaging for editor ↔ RTC monitor (AGENTS.md). */

export const LASERDESK_RTC_CHANNEL = 'laserdesk-rtc-v1';

export type RtcChannelMessage =
	| { type: 'log'; line: string; ts: number }
	| { type: 'request-status' };

export function openRtcChannel(): BroadcastChannel {
	return new BroadcastChannel(LASERDESK_RTC_CHANNEL);
}

/**
 * Broadcast one log line to every listener on {@link LASERDESK_RTC_CHANNEL} (e.g. /rtc tab).
 * Uses a short-lived channel per call so sending still works if a page failed to keep a persistent sender open.
 */
export function postRtcLog(line: string): void {
	if (import.meta.env.DEV) {
		console.debug(`[laserdesk-rtc-log] ${line}`);
	}
	const msg: RtcChannelMessage = { type: 'log', line, ts: Date.now() };
	try {
		const ch = new BroadcastChannel(LASERDESK_RTC_CHANNEL);
		ch.postMessage(msg);
		ch.close();
	} catch {
		/* unsupported or blocked */
	}
}
