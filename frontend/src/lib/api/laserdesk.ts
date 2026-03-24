import { getApiBase } from './config';
import type { SceneV1 } from '$lib/scene/sceneV1';

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
	dxf_line_count?: number;
	dxf_source_name?: string;
	/** Mock: last upload hint; real RTC: usually omitted */
	correction_file_hint?: string;
	/** Ethernet RIF diagnostics since connect */
	rif_udp_timeout_count?: number;
	rif_udp_spurious_datagrams?: number;
	rif_connect_status_retries_used?: number;
	/** Ethernet: first argument of R_DC_EXECUTE_LIST_POS used for this session */
	rif_execute_list_no?: number;
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

export interface DiscoveredRtcHost {
	ip: string;
	remote_status: number;
	remote_pos: number;
}

export interface RtcDiscoverResponse {
	hosts: DiscoveredRtcHost[];
	scanned: number;
	elapsed_ms: number;
}

/** UDP scan: R_DC_GET_STATUS on each host in the IPv4 subnet (backend; same handshake as ethernet connect). */
export async function postRtcDiscover(
	body: {
		base_ip: string;
		netmask: string;
		port?: number;
		tgm_format?: number;
		timeout_ms?: number;
		max_hosts?: number;
	},
	init?: { signal?: AbortSignal }
): Promise<RtcDiscoverResponse> {
	const res = await apiFetch(`${getApiBase()}/rtc/discover`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(body),
		signal: init?.signal
	});
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<RtcDiscoverResponse>;
}

export async function postRtcDisconnect(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/rtc/disconnect`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

/** `R_DC_STOP_EXECUTION` — same as DXF/minimal job stop; no job id in URL. */
export async function postRtcStop(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/rtc/stop`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

/** Multipart: `file` + optional fields table_no, dim, head_a, head_b, number_of_tables, finalize_arg3 (strings). */
export async function postRtcCorrectionLoad(form: FormData): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/rtc/correction/load`, {
		method: 'POST',
		body: form
	});
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

/** Optional `repeatCount` → query `repeat_count` (1..1e6); default 1. Omits query when 1. */
export async function postMinimalDemoRun(repeatCount: number = 1): Promise<void> {
	const n = Math.min(1_000_000, Math.max(1, Math.floor(Number(repeatCount)) || 1));
	const q = n <= 1 ? '' : `?repeat_count=${encodeURIComponent(String(n))}`;
	const res = await apiFetch(`${getApiBase()}/jobs/minimal-demo/run${q}`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

export async function postMinimalDemoStop(): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/jobs/minimal-demo/stop`, { method: 'POST' });
	if (!res.ok) throw new Error(await readError(res));
}

export interface DxfJobEntity {
	index: number;
	type: string;
	layer: string;
	x0: number;
	y0: number;
	z0: number;
	x1: number;
	y1: number;
	z1: number;
	length: number;
}

export interface DxfJobResponse {
	job_id: string;
	source_name: string;
	kind: string;
	line_count: number;
	entities: DxfJobEntity[];
}

export async function postJobsDxfDemo(): Promise<{ job_id: string }> {
	const res = await apiFetch(`${getApiBase()}/jobs/dxf`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ source: 'demo' })
	});
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<{ job_id: string }>;
}

/** Konva scene editor → same job storage as DXF (`GET /jobs/dxf/{id}`, load/run/stop). */
export async function postJobsScene(body: SceneV1): Promise<{ job_id: string }> {
	const res = await apiFetch(`${getApiBase()}/jobs/scene`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(body)
	});
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<{ job_id: string }>;
}

export async function postJobsDxfText(dxfText: string, sourceName: string): Promise<{ job_id: string }> {
	const res = await apiFetch(`${getApiBase()}/jobs/dxf`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ dxf_text: dxfText, source_name: sourceName })
	});
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<{ job_id: string }>;
}

export async function getJobsDxf(jobId: string): Promise<DxfJobResponse> {
	const res = await apiFetch(`${getApiBase()}/jobs/dxf/${encodeURIComponent(jobId)}`);
	if (!res.ok) throw new Error(await readError(res));
	return res.json() as Promise<DxfJobResponse>;
}

export async function postJobsDxfLoad(jobId: string): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/jobs/dxf/${encodeURIComponent(jobId)}/load`, {
		method: 'POST'
	});
	if (!res.ok) throw new Error(await readError(res));
}

/** Optional `repeatCount` → query `repeat_count` (1..1e6); default 1. */
export async function postJobsDxfRun(jobId: string, repeatCount: number = 1): Promise<void> {
	const n = Math.min(1_000_000, Math.max(1, Math.floor(Number(repeatCount)) || 1));
	const q = n <= 1 ? '' : `?repeat_count=${encodeURIComponent(String(n))}`;
	const res = await apiFetch(`${getApiBase()}/jobs/dxf/${encodeURIComponent(jobId)}/run${q}`, {
		method: 'POST'
	});
	if (!res.ok) throw new Error(await readError(res));
}

export async function postJobsDxfStop(jobId: string): Promise<void> {
	const res = await apiFetch(`${getApiBase()}/jobs/dxf/${encodeURIComponent(jobId)}/stop`, {
		method: 'POST'
	});
	if (!res.ok) throw new Error(await readError(res));
}
