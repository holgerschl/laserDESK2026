/** Cross-window messaging for editor ↔ RTC monitor (AGENTS.md). */

export const LASERDESK_RTC_CHANNEL = 'laserdesk-rtc-v1';

export type RtcChannelMessage =
	| { type: 'log'; line: string; ts: number }
	| { type: 'request-status' };

export function openRtcChannel(): BroadcastChannel {
	return new BroadcastChannel(LASERDESK_RTC_CHANNEL);
}

export function postRtcLog(ch: BroadcastChannel | null, line: string): void {
	if (!ch) return;
	const msg: RtcChannelMessage = { type: 'log', line, ts: Date.now() };
	try {
		ch.postMessage(msg);
	} catch {
		/* closed or unsupported */
	}
}
