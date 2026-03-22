import { getApiBase } from './config';

/** Fetch with clearer errors when HTTPS UI cannot reach http://127.0.0.1 (CORS / Private Network Access). */
async function apiFetch(url: string, init?: RequestInit): Promise<Response> {
	try {
		return await fetch(url, init);
	} catch (e) {
		const msg = e instanceof Error ? e.message : String(e);
		const domNet =
			typeof DOMException !== 'undefined' &&
			e instanceof DOMException &&
			(e.name === 'NetworkError' || e.name === 'SecurityError');
		const networkLike =
			e instanceof TypeError ||
			domNet ||
			/Failed to fetch|NetworkError|network error|Load failed|NETWORK_FAILED/i.test(msg);
		if (networkLike) {
			const origin =
				typeof window !== 'undefined' ? `${window.location.protocol}//${window.location.host}` : '';
			const host = url.replace(/\/api\/v1.*$/, '') || url;
			throw new Error(
				`Cannot reach the backend (${host}). ` +
					`If you opened this app from another site (e.g. GitHub Pages), run laserdesk_backend with ` +
					`LASERDESK_CORS_ORIGIN set exactly to this page’s origin (e.g. ${origin || 'https://your-user.github.io'}, no path). ` +
					`On startup the console must mention “Access-Control-Allow-Private-Network=true”; if not, replace the exe with the ` +
					`latest from GitHub Releases. Match the API base port. If Chromium asks to allow local network access, choose Allow. (${msg})`
			);
		}
		throw e;
	}
}

export interface HealthResponse {
	status: string;
	rtc_mode?: string;
}

export interface RtcStatusJson {
	connection_state: string;
	rtc_mode?: string | null;
	package_version_reported?: string;
	bios_eth_reported?: string;
	alignment_ok?: boolean;
	remote_status?: number;
	remote_pos?: number;
	last_error?: { code: string; message: string };
}

export interface ApiErrorBody {
	code: string;
	message: string;
	details?: Record<string, unknown>;
}

async function readError(res: Response): Promise<string> {
	try {
		const j = (await res.json()) as ApiErrorBody;
		return j.message || j.code || res.statusText;
	} catch {
		return res.statusText;
	}
}

export async function getHealth(): Promise<HealthResponse> {
	const res = await apiFetch(`${getApiBase()}/health`);
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<HealthResponse>;
}

export async function getRtcStatus(): Promise<RtcStatusJson> {
	const res = await apiFetch(`${getApiBase()}/rtc/status`);
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<RtcStatusJson>;
}

export async function postRtcConnect(body: Record<string, unknown>): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/rtc/connect`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(body)
	});
	if (!res.ok) throw new Error(await readError(res));
}

export async function postRtcDisconnect(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/rtc/disconnect`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

export async function postMinimalDemoJob(label?: string): Promise<{ job_id: string }> {
	const res = await fetch(`${getApiBase()}/jobs/minimal-demo`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(label ? { label } : {})
	});
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<{ job_id: string }>;
}

export async function postMinimalDemoRun(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/jobs/minimal-demo/run`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

export async function postMinimalDemoStop(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/jobs/minimal-demo/stop`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}
