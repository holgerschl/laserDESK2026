import { browser } from '$app/environment';
import { env } from '$env/dynamic/public';

/** localStorage key for full API root, e.g. `http://127.0.0.1:8080/api/v1` */
export const API_BASE_STORAGE_KEY = 'laserdesk_api_base_v1';

const DEFAULT_RELATIVE = '/api/v1';

/**
 * REST API root (no trailing slash). On GitHub Pages this must be an absolute URL to the machine
 * running `laserdesk_backend` (see Usage page + CORS).
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
	return DEFAULT_RELATIVE;
}
