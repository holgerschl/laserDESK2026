import { browser } from '$app/environment';
import { env } from '$env/dynamic/public';

/** localStorage key for full API root, e.g. `http://127.0.0.1:8080/api/v1` */
export const API_BASE_STORAGE_KEY = 'laserdesk_api_base_v1';

const DEFAULT_RELATIVE = '/api/v1';

/** Default when the UI is served from a host other than this machine (e.g. GitHub Pages). */
export const DEFAULT_LOCAL_BACKEND_API_BASE = 'http://127.0.0.1:8080/api/v1';

function useRelativeApiDefault(): boolean {
	if (!browser) return true;
	const h = window.location.hostname;
	return h === 'localhost' || h === '127.0.0.1' || h === '[::1]' || h === '::1';
}

/**
 * REST API root (no trailing slash).
 * - Dev on localhost: `/api/v1` (Vite proxies to the backend).
 * - Hosted (e.g. github.io): `http://127.0.0.1:8080/api/v1` unless overridden in localStorage / env
 *   (a bare `/api/v1` would hit the Pages host, not your PC).
 */
export function getApiBase(): string {
	if (browser) {
		try {
			const s = localStorage.getItem(API_BASE_STORAGE_KEY)?.trim();
			if (s) return s.replace(/\/$/, '');
		} catch {
			/* private mode */
		}
	}
	const e =
		typeof env.PUBLIC_LASERDESK_API_BASE === 'string' ? env.PUBLIC_LASERDESK_API_BASE.trim() : '';
	if (e) return e.replace(/\/$/, '');
	if (browser && !useRelativeApiDefault()) {
		return DEFAULT_LOCAL_BACKEND_API_BASE.replace(/\/$/, '');
	}
	return DEFAULT_RELATIVE;
}
